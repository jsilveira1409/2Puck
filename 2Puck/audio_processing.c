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
#include "music.h"
#include <leds.h>

/*Uncomment to print the notes recorded*/
#define DEBUGGING
#define MAX_ACCEPTABLE_ERROR	8
#define RESOLUTION  			(I2S_AUDIOFREQ_16K/2)/(FFT_SIZE/2)
#define FREQ_INDEX_OFFSET 		(-2)
#define NB_SAMPLES				160
#define MAX_VOLUME  			1200
#define MIN_VOLUME 				1000
#define OVERLAP_FACTOR	  		(2)	//50%
#define OVERLAP_BUFFER_SIZE		(2*FFT_SIZE/OVERLAP_FACTOR)
#define OVERLAP_INDEX 			(2*FFT_SIZE*(OVERLAP_FACTOR - 1)/OVERLAP_FACTOR)

/* FIR-Decimation constants*/
#define DECIMATION_FACTOR		2
#define NUM_TAPS				29
#define BLOCK_SIZE				32
#define STATE_ARRAY_SIZE		(BLOCK_SIZE + NUM_TAPS - 1)
#define NUM_BLOCKS				(FFT_SIZE/64)


static BSEMAPHORE_DECL(sem_finished_playing, TRUE);
static BSEMAPHORE_DECL(sem_note_played, TRUE);


static float fundamental_frequency(float* data){

//	float decimated_data[FFT_SIZE/2];
//	for(uint16_t i = 0; i < (FFT_SIZE/2); i++){
//		decimated_data[i] = data[2*i];
//	}
//	arm_mult_f32(data,decimated_data,data, FFT_SIZE/2);

	float max_freq_mag = 0;
	uint32_t max_index = 0;
	arm_max_f32(data, (FFT_SIZE/2), &max_freq_mag, &max_index);
	float freq = (float)RESOLUTION*((float)(max_index + FREQ_INDEX_OFFSET ));
	return freq;
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
	}else if(mic_volume < MIN_VOLUME){
		state = 0;
		return 0;
	}

	return 0;
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

	static float micLeft_cmplx_input[2 * FFT_SIZE];
	static float micLeft_output[FFT_SIZE];
	static float overlapping_samples[OVERLAP_BUFFER_SIZE];

	uint8_t status = 0;

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
			float freq = fundamental_frequency(micLeft_output);
			music_send_freq(freq);
			register_note = 0;
		}
		nb_samples = OVERLAP_BUFFER_SIZE;
		nb_overlap_samples = 0;
		arm_copy_f32(overlapping_samples, micLeft_cmplx_input, OVERLAP_BUFFER_SIZE);
	}
}


/*
 * Public Functions
 */

void wait_note_played(void){
	chBSemWait(&sem_note_played);
}
void wait_finish_playing(void){
	chBSemWait(&sem_finished_playing);
}
