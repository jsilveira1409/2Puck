#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <motors.h>
#include <audio/custom_microphone.h>
#include <audio_processing.h>
#include <communications.h>
#include <fft.h>
#include <arm_math.h>

#define RESOLUTION  			(I2S_AUDIOFREQ_16K/2)/(FFT_SIZE/2)
#define HIGH_MAG_THRESHOLD		1000000
#define LOW_MAG_THRESHOLD		550000
#define FREQ_INDEX_OFFSET 		(-2)
#define MIN_FREQ				10
#define MAX_FREQ				(FFT_SIZE - 30)
#define NB_NOTES				36
#define NB_SAMPLES				160
#define RECORDING_SIZE			20


static float micLeft_cmplx_input[2 * FFT_SIZE];
static float micLeft_output[FFT_SIZE];
static float freq = 0;

static uint8_t played_note[RECORDING_SIZE];
static uint16_t discret_freq = 0;
static uint32_t magnitude = 0;

static const uint16_t note_frequency[NB_NOTES] = {
/* 	  A   A#    B    C     C#    D     D#     E     F    F#     G   G#*/

	220, 233, 247,  262,  277,  294,  311,  330,  349,  370,  392,  415,
	440, 466, 494,  523,  554,  587,  622,  659,  698,  740,  784,  831,
	880, 932, 988, 1047, 1108, 1174, 1244, 1318, 1396, 1480, 1568, 1662
};


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

	for(uint16_t i = 0 ; i < num_samples ; i+=4){
		micLeft_cmplx_input[nb_samples] = (float)data[i + MIC_LEFT];
		nb_samples++;
		micLeft_cmplx_input[nb_samples] = 0;
		nb_samples++;
		if(nb_samples >= (2 * FFT_SIZE)){
			break;
		}
	}


	//chprintf((BaseSequentialStream *)&SD3, "%d %f %d \r \n", register_note, freq, get_mic_volume());
	status = note_volume(data, num_samples);
	if(status == 1 && register_note == 0){
		register_note = 1;
	}
	if(nb_samples >= (2 * FFT_SIZE)){
		if(register_note == 1){
			doCmplxFFT_optimized(FFT_SIZE, micLeft_cmplx_input);
			arm_cmplx_mag_f32(micLeft_cmplx_input, micLeft_output, FFT_SIZE);
			//fundamental_frequency(micLeft_output, 4);
			frequency_to_note(micLeft_output);
			register_note = 0;
		}
		nb_samples = 0;
	}
}



void frequency_to_note(float* data){
	static uint8_t state = 0;
	float max_freq_mag = 0;
	uint32_t max_index = 0;

	arm_max_f32(data, (FFT_SIZE/2), &max_freq_mag, &max_index);
	check_smallest_error(&max_index);

//	if(max_freq_mag > HIGH_MAG_THRESHOLD){
//		if(state == 0){
//			state = 1;
//			check_smallest_error(&max_index);
//		}else if (max_freq_mag < LOW_MAG_THRESHOLD){
//			state = 0;
//		}

	max_index = max_index%12;
	find_note(max_index);
	record_note(max_index);
//	}
}


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

void record_note(const uint8_t note_index){
	static uint16_t current_index = 0;
	played_note[current_index] = note_index;

	if(current_index < RECORDING_SIZE){
		current_index ++;
	}else{
		chprintf((BaseSequentialStream *)&SD3, "\n Recording play back \r \n");
		for(uint16_t i = 0; i<RECORDING_SIZE; i++){
			find_note(played_note[i]);
			current_index = 0;
		}
		chprintf((BaseSequentialStream *)&SD3, "\r \n");
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

uint8_t* get_recording_buffer(){
	return played_note;
}

