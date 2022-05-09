#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>
#include <motors.h>
#include <audio/microphone.h>
#include <audio_processing.h>
#include <communications.h>
#include <fft.h>
#include <arm_math.h>
#include <audio_processing.h>

/*Uncomment to print the notes recorded*/
#define DEBUGGING
#define MAX_ACCEPTABLE_ERROR	8
#define RESOLUTION  			(I2S_AUDIOFREQ_16K/2)/(FFT_SIZE/2)
#define FREQ_INDEX_OFFSET 		(-2)
#define NB_SAMPLES				160
#define RECORDING_SIZE			18
#define NB_MICS 				2
#define MAX_VOLUME  			900
#define MIN_VOLUME 				500
#define OVERLAP_FACTOR	  		(2)	//50%
#define OVERLAP_BUFFER_SIZE		(2*FFT_SIZE/OVERLAP_FACTOR)
#define OVERLAP_INDEX 			(2*FFT_SIZE*(OVERLAP_FACTOR - 1)/OVERLAP_FACTOR)


static BSEMAPHORE_DECL(sem_finished_playing, TRUE);
/*
 * Probably need a mutex here, and put pathing with a higger priority,
 * so microphone inherits pathing's priority once it is dancing,
 * in which it waits for the note to be played
 */
static BSEMAPHORE_DECL(sem_note_played, TRUE);

static float micLeft_cmplx_input[2 * FFT_SIZE];
static float micLeft_output[FFT_SIZE];
/*
 * The last samples are taken from one FFT bin and put into the start of the next one,
 * to avoid cutting a note detection into two
 */
static float overlapping_samples[OVERLAP_BUFFER_SIZE];
static float freq = 0;

static uint8_t played_note[RECORDING_SIZE];
static uint16_t current_index = 0;
static uint16_t discret_freq = 0;


/*
 * Prints the discrete note, used for debugging
 */
#ifdef	DEBUGGING
 static void find_note (int16_t index){
	switch (index){
		case 0:
			chprintf((BaseSequentialStream *)&SD3, "A ");
			break;
		case 1:
			chprintf((BaseSequentialStream *)&SD3, "A# ");
			break;
		case 2:
			chprintf((BaseSequentialStream *)&SD3, "B ");
			break;
		case 3:
			chprintf((BaseSequentialStream *)&SD3, "C ");
			break;
		case 4:
			chprintf((BaseSequentialStream *)&SD3, "C# ");
			break;
		case 5:
			chprintf((BaseSequentialStream *)&SD3, "D ");
			break;
		case 6:
			chprintf((BaseSequentialStream *)&SD3, "D# ");
			break;
		case 7:
			chprintf((BaseSequentialStream *)&SD3, "E ");
			break;
		case 8:
			chprintf((BaseSequentialStream *)&SD3, "F ");
			break;
		case 9:
			chprintf((BaseSequentialStream *)&SD3, "F# ");
			break;
		case 10:
			chprintf((BaseSequentialStream *)&SD3, "G ");
			break;
		case 11:
			chprintf((BaseSequentialStream *)&SD3, "G# ");
			break;
		case 12:
			chprintf((BaseSequentialStream *)&SD3, "none  \r ");
			break;
	}
}
#endif

/*
 * Finds the smallest error between the FFT data
 * and the discrete note frequency in note_frequency[]
 *
 *	TODO: implement something that ignores the note when the error is too big,
 *	could help with resolution
 *
 */
static void check_smallest_error(uint32_t *max_index){
	float smallest_error = 0, curr_error = 0;
	freq = (float)RESOLUTION*((float)(*max_index + FREQ_INDEX_OFFSET ));

	smallest_error = abs(freq - (float)note_frequency[0]);
	for(uint8_t i = 1; i<NB_NOTES; i++){
		curr_error = abs(freq - (float)note_frequency[i]);
		if(curr_error < smallest_error){
			smallest_error = curr_error;
			if(smallest_error < MAX_ACCEPTABLE_ERROR){
				discret_freq = note_frequency[i];
				*max_index = i;
			}
		}
	}
}

/*
 * TODO :CHECK IF THERE IS A CMSIS FUNCTION FOR THIS
 */
static void fundamental_frequency(float* data, uint8_t nb_harmonic){
	if(nb_harmonic > 1){
		for(uint8_t harmonic = 1; harmonic <= nb_harmonic;harmonic++ ){
			for(uint16_t i = 0; i< FFT_SIZE; i++){
				data[i] += data[i]/(float)harmonic;
			}
		}
	}
}

uint8_t note_volume(int16_t *data, uint16_t num_samples){
	static uint8_t state = 0;
	static uint16_t mic_volume = 0;

	int16_t max_value = INT16_MIN, min_value = INT16_MAX;

	for(uint16_t i=0; i<num_samples; i+=2) {
		if(data[i + MIC_LEFT] > max_value) {
			max_value = data[i + MIC_LEFT];
		}
		if(data[i + MIC_LEFT] < min_value) {
			min_value = data[i + MIC_LEFT];
		}
	}

	mic_volume = max_value - min_value;

	if(mic_volume > MAX_VOLUME){
		if(state == 0){
			state = 1;
			return 1;
		}else if(state == 1){
			return 0;
		}
	}else if (mic_volume < MIN_VOLUME){
		state = 0;
		return 0;
	}

	return 0;
}

/*
 * Records the played note into the recording array,
 * once we fill the limit, it signals the semaphore to activate the
 * scoring of the recording in music.c
 */
static void record_note(const uint8_t note_index){
	played_note[current_index] = note_index;
	/*
	 * Signals pathing.c that it can dance once
	 */
	chBSemSignal(&sem_note_played);

	if(current_index < (RECORDING_SIZE-1)){
		current_index ++;
	}else{
		current_index = 0;
		chBSemSignal(&sem_finished_playing);
	}
}

/*
 * Receives the FFT data, finds the closest discrete error and
 * records it
 */
static void frequency_to_note(float* data){
	float max_freq_mag = 0;
	uint32_t max_index = 0;

	arm_max_f32(data, (FFT_SIZE/2), &max_freq_mag, &max_index);
	check_smallest_error(&max_index);
	max_index = max_index%12;
#ifdef DEBUGGING
	find_note(max_index);
#endif
	record_note(max_index);
}

/*
*	Callback called when the demodulation of the four microphones is done.
*	We get 160 samples per mic every 10ms (16kHz)
*
*	params :
*	int16_t *data			Buffer containing 4 times 160 samples. the samples are sorted by micro
*							so we have [micRight1, micLeft1, micBack1, micFront1, micRight2, etc...]
*	uint16_t num_samples	Tells how many data we get in total (should always be 640)
*/
void processAudioDataCmplx(int16_t *data, uint16_t num_samples){
	static uint16_t nb_samples = 0;
	static uint8_t register_note = 0;
	static uint16_t nb_overlap_samples = 0;

	uint8_t status = 0;
	/*
	 * Fills the input buffer with the received samples
	 */
	for(uint16_t i = 0 ; i < num_samples ; i+=4){
		micLeft_cmplx_input[nb_samples] = (float)data[i + MIC_LEFT];
		if(nb_samples >= OVERLAP_INDEX){
			overlapping_samples[nb_overlap_samples] =(float)data[i + MIC_LEFT];
			nb_overlap_samples++;
		}
		nb_samples++;
		micLeft_cmplx_input[nb_samples] = 0;
		if(nb_samples >= OVERLAP_INDEX){
			overlapping_samples[nb_overlap_samples] = 0;
			nb_overlap_samples++;
		}
		nb_samples++;
		if(nb_samples >= (2 * FFT_SIZE)){
			break;
		}
	}

	/*
	 * Checks if one (or more) sample has a volume higher than
	 * that of the threshold. Here we implement a Schmitt Trigger
	 */
	status = note_volume(data, num_samples);
	if(status == 1 && register_note == 0){
		register_note = 1;
	}

	/*
	 * Once we have enough samples, and we know that one of the samples
	 * has a higher volume, we do a FFT and discretize it with frequency_to_note
	 */
	if(nb_samples >= (2 * FFT_SIZE)){
		if(register_note == 1){
			doCmplxFFT_optimized(FFT_SIZE, micLeft_cmplx_input);
			arm_cmplx_mag_f32(micLeft_cmplx_input, micLeft_output, FFT_SIZE);
			fundamental_frequency(micLeft_output, 3);
			frequency_to_note(micLeft_output);
			register_note = 0;
		}
		nb_samples = OVERLAP_BUFFER_SIZE;
		nb_overlap_samples = 0;
		arm_copy_f32(overlapping_samples,micLeft_cmplx_input, OVERLAP_BUFFER_SIZE);
	}
}


/*
 * Public Functions
 */
uint8_t* get_recording(void){
	return played_note;
}
uint8_t get_current_last_note(void){
	return played_note[current_index];
}
void wait_note_played(void){
	chBSemWait(&sem_note_played);
}
void wait_finish_playing(void){
	chBSemWait(&sem_finished_playing);
}
