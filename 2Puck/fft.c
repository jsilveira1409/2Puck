#include "ch.h"
#include "hal.h"
#include <main.h>
#include <math.h>
#include <fft.h>

#include <arm_math.h>
#include <arm_const_structs.h>

static arm_rfft_fast_instance_f32 fft_handler;

/*
*	Wrapper to call a very optimized fft function provided by ARM
*	which uses a lot of tricks to optimize the computations
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

void doRealFFT_optimized(float* in_buffer, float* out_buffer){
	arm_rfft_fast_f32(&fft_handler, in_buffer, out_buffer, 0);
}

void init_rfft_handler(uint16_t fft_size){
	arm_rfft_fast_init_f32(&fft_handler, fft_size);
}
