/*
 * @file 		audio_processing.c
 * @brief		Microphone sample processing library.
 * @author		Karl Khalil
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		12 Apr 2022
 * @copyright	GNU Public License
 *
 */

#include <audio/microphone.h>
#include <fft.h>
#include <arm_math.h>
#include "music.h"
#include "audio_processing.h"

#define RESOLUTION  			(I2S_AUDIOFREQ_16K/2)/(FFT_SIZE/2)
#define FREQ_INDEX_OFFSET 		(-2)
#define MAX_VOLUME  			1200
#define MIN_VOLUME 				500
#define OVERLAP_FACTOR	  		(2)	//50%
#define OVERLAP_BUFFER_SIZE		(2*FFT_SIZE/OVERLAP_FACTOR)
#define OVERLAP_INDEX 			(2*FFT_SIZE*(OVERLAP_FACTOR - 1)/OVERLAP_FACTOR)
#define NB_HARMONICS			2
#define FFT_SIZE 				4096

/*
 * @brief		Find fundamental frequency of played note
 * @details		Applies the harmonic product spectrum and finds the maximum value and
 * 				index of the given data
 *
 * @param[in,out] data	FFT data of the microphone
 * @return 				Fundamental frequency of the note played.
 */
static float fundamental_frequency(float* data){
	for(uint8_t i = 2; i < NB_HARMONICS; i+=2){
		float decimated_data[FFT_SIZE/i];
		for(uint16_t j = 0; j < (FFT_SIZE/i); j++){
			decimated_data[j] = data[i*j];
		}
		arm_mult_f32(data,decimated_data,data, FFT_SIZE/i);
	}

	float max_freq_mag = 0;
	uint32_t max_index = 0;
	arm_max_f32(data, (FFT_SIZE/2), &max_freq_mag, &max_index);
	float freq = (float)RESOLUTION*((float)(max_index + FREQ_INDEX_OFFSET ));
	return freq;
}

/*
 * @brief		Check if volume ceiling is passed.
 * @details 	Measures the volume of the microphone's data and
 * 				applies a schmitt trigger to it, and returns true
 * 				if there is a rising edge.
 *
 * @param[in,out] data 			time-domaine microphone data
 * @param[in] num_samples		number of samples in data to analyze
 * @return 						Whether there is a rising edge or not.
 */
bool note_volume(int16_t *data, uint16_t num_samples){
	static bool state = false;
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
		if(state == false){
			state = true;
			return true;
		}else if(state == true){
			return false;
		}
	}else if(mic_volume < MIN_VOLUME){
		state = false;
		return false;
	}
	return false;
}

/*
 * @brief		Process the microphone data.
 * @details 	Callback function called when the demodulation of the four microphones is done. We get 160 samples
 * 				per mic every 10ms (16kHz).
 *
 * @param[in/out] data		buffer containing 4 times 160 samples, sorted by microphone
 * 							[micRight1, micLeft1, micBack1, micFront1, micRight2, etc...]
 * @param[in] num_samples	size of the buffer received
 *
*/
void processAudioDataCmplx(int16_t *data, uint16_t num_samples){
	if(music_is_playing()){
		static uint16_t nb_samples = 0;
		static uint16_t nb_overlap_samples = 0;
		static bool register_note = false;

		static float micLeft_cmplx_input[2 * FFT_SIZE];
		static float micLeft_output[FFT_SIZE];
		static float overlapping_samples[OVERLAP_BUFFER_SIZE];

		bool status = false;

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
		if(status == true && register_note == false){
			register_note = true;
		}

		/*
		 * Once we have enough samples, and we know that one of the samples
		 * has a higher volume, we do a FFT and discretize it with frequency_to_note
		 */
		if(nb_samples >= (2 * FFT_SIZE)){
			if(register_note == true){
				doCmplxFFT_optimized(FFT_SIZE, micLeft_cmplx_input);
				arm_cmplx_mag_f32(micLeft_cmplx_input, micLeft_output, FFT_SIZE);
				float freq = fundamental_frequency(micLeft_output);
				music_send_freq(freq);
				register_note = false;

			}
			nb_samples = OVERLAP_BUFFER_SIZE;
			nb_overlap_samples = 0;
			arm_copy_f32(overlapping_samples, micLeft_cmplx_input, OVERLAP_BUFFER_SIZE);
		}
	}
}
