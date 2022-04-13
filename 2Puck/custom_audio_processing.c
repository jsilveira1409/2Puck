#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <motors.h>
#include <audio/custom_microphone.h>
#include <custom_audio_processing.h>
#include <communications.h>
#include <fft.h>
#include <arm_math.h>

#define FREQ_OFFSET 		(-2)				//voir pourquoi il en faut???
#define MIN_VALUE_THRESHOLD 400000
#define MIN_FREQ 			0
#define MAX_FREQ 			FFT_SIZE/2
#define NB_NOTES 			36
#define RESOLUTION  (CUSTOM_I2S_AUDIOFREQ/2)/(FFT_SIZE/2)

//semaphore
static BSEMAPHORE_DECL(sendToComputer_sem, TRUE);

//2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
static float micLeft_cmplx_input[2 * FFT_SIZE];

//Arrays containing the computed magnitude of the complex numbers
static float micLeft_output[FFT_SIZE];

static float freq;
static uint16_t discret_freq = 0;

//send_tab is used to save the state of the buffer to send (double buffering)
//to avoid modifications of the buffer while sending it
static float send_tab[FFT_SIZE];

/*LUP for the note frequency in the guitar fretboard,total of 3 octaves : 3*12 = 36
 * starting with the open string E, ending with fret 12 of string 6.
 *
 */
static const uint16_t note_frequency[NB_NOTES] = {
/*    E    F    F#   G   G#    A   A#    B    C   C#   D   D#    */
	 82,  87,  92,  98, 104, 110, 117, 123, 131, 139, 147, 156,
	165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311,
	330, 349, 370, 392, 415, 440, 466, 494, 554, 587, 622, 659
};


void fundamental_frequency(float* data, uint8_t nb_harmonic){
	for(uint8_t harmonic = 1; harmonic<=nb_harmonic;harmonic++ ){
		for(uint16_t i=0; i<FFT_SIZE; i++){
			data[i] += data[i]/(float)harmonic;
		}
	}
}

void frequency_to_note(float* data){
	volatile float max_norm = MIN_VALUE_THRESHOLD;
	volatile float smallest_error = 0, curr_error = 0;
	volatile int16_t max_norm_index = -1, note_index = 0;

	for(uint16_t i = MIN_FREQ; i<= MAX_FREQ; i++){
		if(data[i] > max_norm){
			max_norm = data[i];
			max_norm_index = i;
		}
	}
	if(max_norm_index > -1){
		freq = RESOLUTION*((float)(max_norm_index + FREQ_OFFSET));

		smallest_error = abs(freq - (float)note_frequency[0]);

		for(uint8_t i = 1; i<NB_NOTES; i++){
			curr_error = abs(freq - (float)note_frequency[i]);
			if(curr_error < smallest_error){
				smallest_error = curr_error;
				discret_freq = note_frequency[i];
				note_index = i;
			}
		}
		note_index = note_index % 12;
		find_note(note_index);
	}else{
		freq = 0;
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

	for(volatile uint16_t i = 0; i < num_samples; i+=4){			// i counts the input buffer fullfilment
		micLeft_cmplx_input[nb_samples] = (float)data[i + MIC_LEFT];
		nb_samples++;												// each buffer received one sample, so we increment i by one
		micLeft_cmplx_input[nb_samples] = 0;
		nb_samples++;
		if(nb_samples >= (2*FFT_SIZE)){								//filled the input buffers
			break;													//restart the circular buffer
		}
	}

	if(nb_samples >= (2 * FFT_SIZE)){
		doFFT_optimized(FFT_SIZE, micLeft_cmplx_input);
		arm_cmplx_mag_f32(micLeft_cmplx_input, micLeft_output, FFT_SIZE);
		//arm_copy_f32(micLeft_output, send_tab, FFT_SIZE);			//not needed as we don't send large chunks of data to the pc
		fundamental_frequency(micLeft_output, 4);
		frequency_to_note(micLeft_output);
		nb_samples = 0;
		chBSemSignal(&sendToComputer_sem);							//signals the buffer is ready to send
	}
}

void send_to_computer(void){
	//chprintf((BaseSequentialStream *)&SD3, "%f \r  %d \r  \n", freq, discret_freq);
}
void wait_send_to_computer(void){
	chBSemWait(&sendToComputer_sem);
}


void find_note (uint16_t index){
	switch (index){
		case 0:
			chprintf((BaseSequentialStream *)&SD3, "E \r \n");
			left_motor_set_speed(1000);
			right_motor_set_speed(1000);
			break;
		case 1:
			chprintf((BaseSequentialStream *)&SD3, "F \r \n");
			left_motor_set_speed(400);
			right_motor_set_speed(1000);
			break;
		case 2:
			chprintf((BaseSequentialStream *)&SD3, "F# \r \n");
			break;
		case 3:
			chprintf((BaseSequentialStream *)&SD3, "G \r \n");
			left_motor_set_speed(1000);
			right_motor_set_speed(400);
			break;
		case 4:
			chprintf((BaseSequentialStream *)&SD3, "G# \r \n");
			break;
		case 5:
			chprintf((BaseSequentialStream *)&SD3, "A \r \n");
			left_motor_set_speed(600);
			right_motor_set_speed(-600);
			break;
		case 6:
			chprintf((BaseSequentialStream *)&SD3, "A# \r \n");
			break;
		case 7:
			chprintf((BaseSequentialStream *)&SD3, "B \r \n");
			left_motor_set_speed(-1000);
			right_motor_set_speed(-1000);
			break;
		case 8:
			chprintf((BaseSequentialStream *)&SD3, "C \r \n");
			left_motor_set_speed(1000);
			right_motor_set_speed(0);
			break;
		case 9:
			chprintf((BaseSequentialStream *)&SD3, "C# \r \n");
			left_motor_set_speed(0);
			right_motor_set_speed(-1000);
			break;
		case 10:
			chprintf((BaseSequentialStream *)&SD3, "D \r \n");
			left_motor_set_speed(700);
			right_motor_set_speed(-200);
			break;
		case 11:
			chprintf((BaseSequentialStream *)&SD3, "D# \r \n");
			left_motor_set_speed(-200);
			right_motor_set_speed(600);
			break;
		case 12:
			chprintf((BaseSequentialStream *)&SD3, "none \r \n");
			left_motor_set_speed(0);
			right_motor_set_speed(0);
			break;

	}
}
