/*
 * @file 		fft.h
 * @brief		FFT wrapping functions.
 * @author		Karl Khalil
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		12 Apr 2022
 * @copyright	GNU Public License
 *
 */
#ifndef FFT_H
#define FFT_H
#include "arm_math.h"

void init_rfft_handler(uint16_t fft_size);
void doCmplxFFT_optimized(uint16_t size, float* complex_buffer);

#endif /* FFT_H */
