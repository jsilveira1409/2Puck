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
#include <leds.h>


#define RESOLUTION  			(I2S_AUDIOFREQ_16K/2)/(FFT_SIZE/2)
#define FREQ_INDEX_OFFSET 		(-2)
#define NB_SAMPLES				160
#define RECORDING_SIZE			20
#define NB_MICS 2


static BSEMAPHORE_DECL(sem_finished_playing, TRUE);

static float micLeft_cmplx_input[2 * FFT_SIZE];
static float micLeft_output[FFT_SIZE];
static float freq = 0;

static const uint16_t MAX_VOLUME = 900, MIN_VOLUME = 800;
static uint16_t mic_volume = 0;
static int16_t mic_last;


static uint8_t played_note[RECORDING_SIZE];
static uint16_t discret_freq = 0;


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

	/*
	*
	*	We get 160 samples per mic every 10ms
	*	So we fill the samples buffers to reach
	*	1024 samples, then we compute the FFTs.
	*
	*/

	static uint16_t nb_samples = 0;
	static uint8_t register_note = 0;
	uint8_t status = 0;
	/*
	 * Fills the input buffer with the received samples
	 */
	for(uint16_t i = 0 ; i < num_samples ; i+=4){
		micLeft_cmplx_input[nb_samples] = (float)data[i + MIC_LEFT];
		nb_samples++;
		micLeft_cmplx_input[nb_samples] = 0;
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
			fundamental_frequency(micLeft_output, 4);
			frequency_to_note(micLeft_output);
			register_note = 0;
		}
		nb_samples = 0;
	}
}

/*
 * Receives the FFT data, finds the closest discret error and
 * records it
 */

void frequency_to_note(float* data){
	float max_freq_mag = 0;
	uint32_t max_index = 0;

	arm_max_f32(data, (FFT_SIZE/2), &max_freq_mag, &max_index);
	check_smallest_error(&max_index);
	max_index = max_index%12;
//	find_note(max_index);
	record_note(max_index);
}

/*
 * Finds the smallest error between the FFT data
 * and the discrete note frequency in note_frequency[]
 *
 *	TODO: implement something that ignores the note when the error is too big,
 *	could help with resolution
 *
 */
void check_smallest_error(uint32_t *max_index){
	float smallest_error = 0, curr_error = 0;
	freq = (float)RESOLUTION*((float)(*max_index + FREQ_INDEX_OFFSET ));

	smallest_error = abs(freq - (float)note_frequency[0]);
	for(uint8_t i = 1; i<NB_NOTES; i++){
		curr_error = abs(freq - (float)note_frequency[i]);
		if(curr_error < smallest_error){
			smallest_error = curr_error;
			discret_freq = note_frequency[i];
			*max_index = i;
		}
	}

}

/*
 * Prints the discrete note
 */
void find_note (int16_t index){
	switch (index){
		case 0:
			chprintf((BaseSequentialStream *)&SD3, "A  ");
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

/*
 * Records the played note into the recording array,
 * once we fill the limit, it signals the mutex to activate the
 * scoring of the recording in music.c
 */
void record_note(const uint8_t note_index){
	static uint16_t current_index = 0;
	static uint8_t led = 0;
	set_led(LED7,led);
	if(led == 1) led = 0;
	else led = 1;
	played_note[current_index] = note_index;

	if(current_index < RECORDING_SIZE){
		current_index ++;
	}else{
		current_index = 0;
		chBSemSignal(&sem_finished_playing);
	}
}

/*
 * CHECK IF THERE IS A CMSIS FUNCTION FOR THIS
 */
void fundamental_frequency(float* data, uint8_t nb_harmonic){
	if(nb_harmonic > 1){
		for(uint8_t harmonic = 1; harmonic <= nb_harmonic;harmonic++ ){
			for(uint16_t i = 0; i< FFT_SIZE; i++){
				data[i] += data[i]/(float)harmonic;
			}
		}
	}
}

uint8_t* get_recording(void){
	return played_note;
}


uint8_t note_volume(int16_t *data, uint16_t num_samples){
	static uint8_t state = 0;

	int16_t max_value=INT16_MIN, min_value=INT16_MAX;

	for(uint16_t i=0; i<num_samples; i+=2) {
		if(data[i + MIC_LEFT] > max_value) {
			max_value = data[i + MIC_LEFT];
		}
		if(data[i + MIC_LEFT] < min_value) {
			min_value = data[i + MIC_LEFT];
		}
	}

	mic_volume = max_value - min_value;
	mic_last = data[MIC_BUFFER_LEN-(NB_MICS-MIC_LEFT)];

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


void wait_finish_playing(void){
	chBSemWait(&sem_finished_playing);
}
