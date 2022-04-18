#ifndef FFT_H
#define FFT_H
#include "arm_math.h"


void init_rfft_handler(uint16_t fft_size);
void doCmplxFFT_optimized(uint16_t size, float* complex_buffer);

void doRealFFT_optimized(float* in_buffer, float* out_buffer);
#endif /* FFT_H */
