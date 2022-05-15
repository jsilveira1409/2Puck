/*
 * @file 		rng.h
 * @brief		Random Number Generator Library
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		1 May 2022
 * @copyright	GNU Public License
 *
 */

#ifndef 	RNG_H_
#define 	RNG_H_

void rng_stop(void);
void rng_init(void);
uint32_t rng_get(void);

#endif /* RNG_H_*/
