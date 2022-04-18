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

//semaphore
static BSEMAPHORE_DECL(sendToComputer_sem, TRUE);

static float micLeft_cmplx_input[2 * FFT_SIZE];
static float micLeft_output[FFT_SIZE];

static uint16_t discret_freq = 0;
static float freq = 0;

#define PI						3.1415
#define RESOLUTION  			(I2S_AUDIOFREQ_16K/2)/(FFT_SIZE/2)
#define MIN_VALUE_THRESHOLD		10000
#define FREQ_INDEX_OFFSET 		(-2.7)
#define MIN_FREQ				10
#define MAX_FREQ				(FFT_SIZE - 30)
#define NB_NOTES				36
#define NB_SAMPLES				160

static float hann_window[2*FFT_SIZE] = {0};

static const uint16_t note_frequency[NB_NOTES] = {
/*    E    F    F#   G   G#    A   A#    B    C   C#   D   D#    */
	 82,  87,  92,  98, 104, 110, 117, 123, 131, 139, 147, 156,
	165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311,
	330, 349, 370, 392, 415, 440, 466, 494, 554, 587, 622, 659
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
	static uint8_t mustSend = 0;
	static uint8_t current_buffer = 0;

	uint8_t register_note = 0;

	//loop to fill the buffers
	for(uint16_t i = 0 ; i < num_samples ; i+=4){
		micLeft_cmplx_input[nb_samples] = (float)data[i + MIC_LEFT];
		nb_samples++;
		micLeft_cmplx_input[nb_samples] = 0;
		nb_samples++;
		if(nb_samples >= (2 * FFT_SIZE)){
			break;
		}
	}


	if(nb_samples >= (2 * FFT_SIZE)){
		doCmplxFFT_optimized(FFT_SIZE, micLeft_cmplx_input);
		arm_cmplx_mag_f32(micLeft_cmplx_input, micLeft_output, FFT_SIZE);

		//register_note = note_volume(data, num_samples);
		//if(register_note == 1){
			//fundamental_frequency(micLeft_output, 2);
			frequency_to_note(micLeft_output);
		//}
		nb_samples = 0;
		chBSemSignal(&sendToComputer_sem);
	}
}


void wait_send_to_computer(void){
	chBSemWait(&sendToComputer_sem);
}

float* get_audio_buffer_ptr(BUFFER_NAME_t name){
	if(name == LEFT_CMPLX_INPUT){
		return micLeft_cmplx_input;
	}
	else if (name == LEFT_OUTPUT){
		return micLeft_output;
	}else{
		return NULL;
	}
}


void frequency_to_note(float* data){
	float max_freq = 0;
	uint32_t max_index = 0;
	float smallest_error = 0, curr_error = 0;

	arm_max_f32(data, (FFT_SIZE/2 - 100), &max_freq, &max_index);

	freq = (float)RESOLUTION*((float)(max_index + FREQ_INDEX_OFFSET ));
	smallest_error = abs(freq - note_frequency[0]);
	for(uint8_t i = 1; i<NB_NOTES; i++){
		curr_error = abs(freq - note_frequency[i]);
		if(curr_error < smallest_error){
			smallest_error = curr_error;
			discret_freq = note_frequency[i];
			max_index = i;
		}
	}

	max_index = max_index%12;
	find_note(max_index);

	chprintf((BaseSequentialStream *)&SD3, "%f %d \r  \n", freq, max_index);
}


void find_note (int16_t index){
	switch (index){
		case 0:
			chprintf((BaseSequentialStream *)&SD3, "E   %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 1:
			chprintf((BaseSequentialStream *)&SD3, "F %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 2:
			chprintf((BaseSequentialStream *)&SD3, "F# %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 3:
			chprintf((BaseSequentialStream *)&SD3, "G %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 4:
			chprintf((BaseSequentialStream *)&SD3, "G# %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 5:
			chprintf((BaseSequentialStream *)&SD3, "A %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 6:
			chprintf((BaseSequentialStream *)&SD3, "A# %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 7:
			chprintf((BaseSequentialStream *)&SD3, "B %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 8:
			chprintf((BaseSequentialStream *)&SD3, "C %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 9:
			chprintf((BaseSequentialStream *)&SD3, "C# %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 10:
			chprintf((BaseSequentialStream *)&SD3, "D %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 11:
			chprintf((BaseSequentialStream *)&SD3, "D# %f  %d  \r  \n", freq, get_mic_volume());
			break;
		case 12:
			chprintf((BaseSequentialStream *)&SD3, "none %f  %d  \r  \n", freq, get_mic_volume());
			break;
	}
}

void fundamental_frequency(float* data, uint8_t nb_harmonic){
	if(nb_harmonic > 1){
		for(uint8_t harmonic = 1; harmonic<=nb_harmonic;harmonic++ ){
			for(uint16_t i = 0; i< FFT_SIZE; i++){
				data[i] += data[i]/(float)harmonic;
			}
		}
	}
}


void init_hann_window(void){
	for(uint16_t i = 0; i<FFT_SIZE;i++){
		hann_window[2*i] = 0.5*(1 - arm_cos_f32(2*PI*(float)i/(float)NB_SAMPLES));
		hann_window[2*i + 1] = 0;								//cmplx data index, don't care
	}
}
/*
 * Windowing function on the sampled data
 */
void window(float* data, uint16_t size){
	arm_dot_prod_f32(data, hann_window, size, data);
}
