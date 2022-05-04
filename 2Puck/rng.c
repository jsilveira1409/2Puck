#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include <rng.h>

/*
 * Public Functions
 */

void rng_init(void) {
	/* Enable RNG clock */
	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;

	/* RNG Peripheral enable */
	RNG->CR |= RNG_CR_RNGEN;
}

void rng_stop(void) {
	/* Disable RNG peripheral */
	RNG->CR &= ~RNG_CR_RNGEN;

	/* Disable RNG clock source */
	RCC->AHB2ENR &= ~RCC_AHB2ENR_RNGEN;
}

uint32_t rng_get(void) {
	/* Wait until one RNG number is ready */
	while (!(RNG->SR & (RNG_SR_DRDY)));

	/* Get a 32-bit Random number */
	return RNG->DR;
}

