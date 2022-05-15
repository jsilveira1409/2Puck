/*
 * rng.h
 *
 *  Created on: 12 Apr 2022
 *      Author: Joaquim Silveira
 */

#ifndef 	RNG_H_
#define 	RNG_H_

void rng_stop(void);
void rng_init(void);
uint32_t rng_get(void);

#endif /* RNG_H_*/
