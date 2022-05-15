/*
 * fft.h
 *
 * 	Created on:
 * 	Authors: Francesco Mondada
 * 		     Daniel Burnier
 */

#include "ch.h"
#include "hal.h"
#include <main.h>
#include <math.h>
#include <fft.h>
#include <arm_math.h>
#include <arm_const_structs.h>

/*
 * @brief 		FFT function calls
 * @details		Wrapper to call a very optimized fft function provided by ARM
 * 				which uses a lot of tricks to optimize the computations
 *
 * @param[in] size 					size of the complex buffer
 * @param[in,out] complex_buffer	complex buffer containing the time-domain data
 * 									on which the FFT is applied.
*/
void doCmplxFFT_optimized(uint16_t size, float* complex_buffer){
	/*
	 * Audio signal is real so it does not make sense to calculate cfft, we'll try rfft
	*/
	  if(size == 1024)
		arm_cfft_f32(&arm_cfft_sR_f32_len1024, complex_buffer, 0, 1);
	  else if (size == 2048)
		arm_cfft_f32(&arm_cfft_sR_f32_len2048, complex_buffer, 0, 1);
	  else if (size == 4096)
		arm_cfft_f32(&arm_cfft_sR_f32_len4096, complex_buffer, 0, 1);

}
