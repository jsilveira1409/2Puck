#ifndef CUSTOM_MICROPHONE_H
#define CUSTOM_MICROPHONE_H


#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>
#include "mp45dt02_processing.h"

/**
 * Microphones position:
 *
 *      FRONT
 *       ###
 *    #   3   #
 *  #           #
 * # 1   TOP   0 #
 * #     VIEW    #
 *  #           #
 *    #   2   #
 *       ###
 *
 */

//position of the microphones in the buffer given to the customFullbufferCb
#define MIC_LEFT 1
#define MIC_RIGHT 0
#define MIC_FRONT 3
#define MIC_BACK 2

/**
 * @brief 	Starts the microphones acquisition and call the customFullbufferCb
 * 			if one is given.
 * 
 * @param customFullbufferCb callback called when 10ms of samples for each mic have been captured
 */
void mic_start(mp45dt02FullBufferCb customFullbufferCb);
uint16_t get_mic_volume(void);
uint8_t note_volume(int16_t *data, uint16_t num_samples);

#ifdef __cplusplus
}
#endif

#endif
