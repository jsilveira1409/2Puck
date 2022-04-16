#ifndef FFT_H
#define FFT_H
#include "arm_math.h"

typedef struct complex_float{
	float real;
	float imag;
}complex_float;

void doFFT_optimized(uint16_t size, float* complex_buffer);

void doFFT_c(uint16_t size, complex_float* complex_buffer);
void doRealFFT_optimized(uint16_t size, float* in_buffer, float* out_buffer, arm_rfft_fast_instance_f32 * fft_handler);
#endif /* FFT_H */
