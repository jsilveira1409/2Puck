#include <string.h>
#include <ch.h>
#include <hal.h>
#include "custom_microphone.h"
#include "mp45dt02_processing.h"

#define NB_MICS 2

static const uint16_t MAX_VOLUME = 900, MIN_VOLUME = 800;
static uint16_t mic_volume = 0;
static int16_t mic_last;


/*
 * Checks whether the note played depasses a certain volume threshold
 * acts like a Schmitt Trigger for volume
 */

uint16_t get_mic_volume(void){
	return mic_volume;
}

uint8_t note_volume(int16_t *data, uint16_t num_samples){
	static uint8_t state = 0;

	int16_t max_value=INT16_MIN, min_value=INT16_MAX;

	for(uint16_t i=0; i<num_samples; i+=2) {
		if(data[i + MIC_LEFT] > max_value) {
			max_value = data[i + MIC_LEFT];
		}
		if(data[i + MIC_LEFT] < min_value) {
			min_value = data[i + MIC_LEFT];
		}
	}

	mic_volume = max_value - min_value;
	mic_last = data[MIC_BUFFER_LEN-(NB_MICS-MIC_LEFT)];

	if(mic_volume > MAX_VOLUME){
		if(state == 0){
			state = 1;
			return 1;
		}else if(state == 1){
			return 0;
		}
	}else if (mic_volume < MIN_VOLUME){
		state = 0;
		return 0;
	}
	return 0;
}
void mic_start(mp45dt02FullBufferCb customFullbufferCb) {

	if(I2SD2.state != I2S_STOP) {
		return;
	}

	// *******************
	// TIMER CONFIGURATION
	// *******************
	// TIM9CH1 => input, the source is the I2S2 clock.
	// TIM9CH2 => output, this is the clock for microphones, 1/2 of input clock.

    rccEnableTIM9(FALSE);
    rccResetTIM9();

    STM32_TIM9->SR   = 0; // Clear eventual pending IRQs.
    STM32_TIM9->DIER = 0; // DMA-related DIER settings => DMA disabled.

    // Input channel configuration.
    STM32_TIM9->CCER &= ~STM32_TIM_CCER_CC1E; // Channel 1 capture disabled.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_CC1S_MASK; // Reset channel selection bits.
    STM32_TIM9->CCMR1 |= STM32_TIM_CCMR1_CC1S(1); // CH1 Input on TI1.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_IC1F_MASK; // No filter.
    STM32_TIM9->CCER &= ~(STM32_TIM_CCER_CC1P | STM32_TIM_CCER_CC1NP); // Rising edge, non-inverted.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_IC1PSC_MASK; // No prescaler

    // Trigger configuration.
    STM32_TIM9->SMCR &= ~STM32_TIM_SMCR_TS_MASK; // Reset trigger selection bits.
    STM32_TIM9->SMCR |= STM32_TIM_SMCR_TS(5); // Input is TI1FP1.
    STM32_TIM9->SMCR &= ~STM32_TIM_SMCR_SMS_MASK; // Reset the slave mode bits.
    STM32_TIM9->SMCR |= STM32_TIM_SMCR_SMS(7); // External clock mode 1 => clock is TI1FP1.

    // Output channel configuration (pwm mode).
    STM32_TIM9->CR1 &= ~STM32_TIM_CR1_CKD_MASK; // No clock division.
    STM32_TIM9->ARR = 1; // Output clock halved.
    STM32_TIM9->PSC = 0; // No prescaler, counter clock frequency = fCK_PSC / (PSC[15:0] + 1).
    STM32_TIM9->EGR = STM32_TIM_EGR_UG; // Enable update event to reload preload register value immediately.
    STM32_TIM9->CCER &= ~STM32_TIM_CCER_CC2E; // Channel 2 output disabled.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_CC2S_MASK; // Reset channel selection bits => channel configured as output.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_OC2M_MASK; // Reset channel mode bits.
    STM32_TIM9->CCMR1 |= STM32_TIM_CCMR1_OC2M(6); // PWM1 mode.
    STM32_TIM9->CCER &= ~(STM32_TIM_CCER_CC2P | STM32_TIM_CCER_CC2NP); // Active high.
    STM32_TIM9->CCR[1] = 1; // Output clock halved.
    STM32_TIM9->CCMR1 |= STM32_TIM_CCMR1_OC2PE; // Enable preload at each update event for channel 2.
    STM32_TIM9->CCER &= ~STM32_TIM_CCMR1_OC2FE; // Disable fast mode.

    // Enable channels.
    STM32_TIM9->CCER |= STM32_TIM_CCER_CC1E | STM32_TIM_CCER_CC2E;
    STM32_TIM9->CR1 |= STM32_TIM_CR1_CEN;

    // ***************************
	// I2S2 AND I2S3 CONFIGURATION
    // ***************************
    mp45dt02Config micConfig;
    memset(&micConfig, 0, sizeof(micConfig));
    if (customFullbufferCb)
        micConfig.fullbufferCb = customFullbufferCb; // Custom callback called when the buffer is filled with 10 ms of PCM data.
    mp45dt02Init(&micConfig);

}
