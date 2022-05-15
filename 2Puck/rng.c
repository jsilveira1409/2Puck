/*
 * @file 		rng.c
 * @brief		Random Number Generator library
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		1 May 2022
 * @copyright	GNU Public License
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "rng.h"
#include "ch.h"
#include "hal.h"


/*
 * Public Functions
 */

/*
 * @brief	 Initializes the RNG's clock and peripheral.
 */
void rng_init(void) {
	/* Enable RNG clock */
	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;

	/* RNG Peripheral enable */
	RNG->CR |= RNG_CR_RNGEN;
}

/*
 * @brief	Stop the RNG's clock and peripheral.
 */
void rng_stop(void) {
	/* Disable RNG peripheral */
	RNG->CR &= ~RNG_CR_RNGEN;

	/* Disable RNG clock source */
	RCC->AHB2ENR &= ~RCC_AHB2ENR_RNGEN;
}

/*
 * @brief 	Returns the random value generated
 *
 * @return	Random 32-bit number.
 */
uint32_t rng_get(void) {
	/* Wait until one RNG number is ready */
	while (!(RNG->SR & (RNG_SR_DRDY)));

	/* Get a 32-bit Random number */
	return RNG->DR;
}

