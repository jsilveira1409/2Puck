#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <motors.h>
#include <audio/microphone.h>
#include <custom_audio_processing.h>
#include <communications.h>
#include <fft.h>
#include <arm_math.h>



#define FREQ_OFFSET 		(-2)				//voir pourquoi il en faut???
#define MIN_VALUE_THRESHOLD 20000
#define MIN_FREQ_INDEX 			50
#define MAX_FREQ_INDEX 			(FFT_SIZE/2  - 100)
#define NB_NOTES 			36
#define RESOLUTION  		(I2S_AUDIOFREQ/2)/(FFT_SIZE)
#define NOTE_BUFFER_SIZE	100

//semaphore
static BSEMAPHORE_DECL(sendToComputer_sem, TRUE);

//Real FFT Handler
static arm_rfft_fast_instance_f32 fft_handler;

//2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
static float micLeft_cmplx_input[FFT_SIZE];

//Arrays containing the computed magnitude of the complex numbers
static float micLeft_output[FFT_SIZE];
static float send_tab[FFT_SIZE];

static float freq = 0;
static uint16_t discret_freq = 0;

/*
 * Circular Buffer to register the played notes
 */
//static uint8_t note_buffer [NOTE_BUFFER_SIZE] = {0};

/*
 * LUP for the note frequency in the guitar fretboard,total of 3 octaves : 3*12 = 36
 * starting with the open string E, ending with fret 12 of string 6.
 */
static const uint16_t note_frequency[NB_NOTES] = {
/*    E    F    F#   G   G#    A   A#    B    C   C#   D   D#    */
	 82,  87,  92,  98, 104, 110, 117, 123, 131, 139, 147, 156,
	165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311,
	330, 349, 370, 392, 415, 440, 466, 494, 554, 587, 622, 659
};


void fundamental_frequency(float* data, uint8_t nb_harmonic){
	if(nb_harmonic > 1){
		for(uint8_t harmonic = 1; harmonic<=nb_harmonic;harmonic++ ){
			for(uint16_t i = 0; i< FFT_SIZE; i++){
				data[i] += data[i]/(float)harmonic;
			}
		}
	}
}

void frequency_to_note(float* data){
	volatile float max_norm = MIN_VALUE_THRESHOLD;
	volatile float smallest_error = 0, curr_error = 0;
	volatile int16_t max_norm_index = -1, note_index = -1;

	for(uint16_t i = 35; i<= FFT_SIZE; i++){
		if(data[i] > max_norm){
			max_norm = data[i];
			max_norm_index = i;
		}
	}
	freq = (float)RESOLUTION*((float)(max_norm_index + FREQ_OFFSET));
	if(max_norm_index > -1){
		smallest_error = abs(freq - (float)note_frequency[0]);

		for(uint8_t i = 1; i<NB_NOTES; i++){
			curr_error = abs(freq - (float)note_frequency[i]);
			if(curr_error < smallest_error){
				smallest_error = curr_error;
				discret_freq = note_frequency[i];
				note_index = i;
			}
		}

		//chprintf((BaseSequentialStream *)&SD3, "%f %d \r  \n", freq, max_norm_index);
		if(note_index != -1){
			note_index = note_index % 12;
			//find_note(note_index);
		}
	}else{
		//freq = 0;
		find_note(12);
	}
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
void processAudioData(int16_t *data, uint16_t num_samples){
	/*
	*
	*	We get 160 samples per mic every 10ms
	*	So we fill the samples buffers to reach
	*	1024 samples, then we compute the FFTs.
	*
	*/
	static uint16_t nb_samples = 0;
	volatile uint8_t register_note = 0;
	/*	COMPLEX Data processing
	 *
	 																//marche pas si  +=2 voir pourquoi
	for(volatile uint16_t i = 0; i < num_samples; i+=2   ){			// i counts the input buffer fullfilment
		micLeft_cmplx_input[nb_samples] = (float)data[i + MIC_LEFT];
		nb_samples++;												// each buffer received one sample, so we increment i by one
		micLeft_cmplx_input[nb_samples] = 0;
		nb_samples++;
		if(nb_samples >= (2*FFT_SIZE)){								//filled the input buffers
			break;													//restart the circular buffer
		}
	}*/
	/*
	 * REAL Data processing
	 */
	for(volatile uint16_t i = 0; i < num_samples; i+=4   ){
		micLeft_cmplx_input[nb_samples] = (float)data[i + MIC_LEFT];
		nb_samples++;
		if(nb_samples >= (FFT_SIZE)){								//filled the input buffers
			break;
		}
	}

	if(nb_samples >= (FFT_SIZE)){
		//register_note = note_volume(data, num_samples);
		//if(register_note == 1){
		doRealFFT_optimized(FFT_SIZE, micLeft_cmplx_input, micLeft_output, &fft_handler);
		//doFFT_optimized(FFT_SIZE, micLeft_cmplx_input);
		arm_cmplx_mag_f32(micLeft_output, micLeft_output, FFT_SIZE);
		//arm_cmplx_mag_f32(micLeft_cmplx_input, micLeft_output, FFT_SIZE);
		//fundamental_frequency(micLeft_output, 4);
		//frequency_to_note(micLeft_output);
		//}
		arm_copy_f32(micLeft_output, send_tab, FFT_SIZE);
		nb_samples = 0;
		chBSemSignal(&sendToComputer_sem);							//signals the buffer is ready to send
	}
}

void send_to_computer(void){
	SendFloatToComputer((BaseSequentialStream *) &SD3, send_tab, FFT_SIZE);
}

void wait_send_to_computer(void){
	chBSemWait(&sendToComputer_sem);
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


void init_rfft_handler(){
	arm_rfft_fast_init_f32(&fft_handler, FFT_SIZE);
}




