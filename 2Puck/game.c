/*
 * game.c
 *
 *  Created on: 29 Apr 2022
 *  Authors: Karl Khalil
 *  		 Joaquim Silveira
 */

#include <ch.h>
#include <hal.h>
#include <leds.h>
#include <audio_processing.h>
#include <photo.h>
#include "communications.h"
#include "music.h"


typedef enum {
	IDLE,
	START_GAME,
	SEND_PHOTO,
	FINISHED
} GAME_STATE;

static THD_WORKING_AREA(gameWA, 128);

static THD_FUNCTION(game_thd, arg) {

	(void) arg;

	GAME_STATE state = IDLE;
	uint8_t score1 = 0;
	uint8_t score2 = 0;
	uint8_t message = 0;
	uint8_t song = 0;

	while(true) {
		switch(state){
			case IDLE:
				message = chSequentialStreamGet(&SD3);
				if (message == 'w'){
					set_body_led(1);
					state++;
				}
				break;

			case START_GAME:
				init_music();
				song = get_song();
				chSequentialStreamWrite(&SD3, &song, 1);
				set_led(LED1, 1);

				wait_finish_music();
				score1 = get_score();
				SendUint8ToComputer(&score1, 1);
				set_led(LED5,1);

				wait_finish_music();
				score2 = get_score();
				SendUint8ToComputer(&score2, 1);
				state++;
				break;

			case SEND_PHOTO:
				init_photo();
				state++;
				break;

			case FINISHED:
				state = IDLE;
				break;
		}
		chThdSleepMilliseconds(200);
	}
}

void game_init(void){
	chThdCreateStatic(gameWA, sizeof(gameWA), NORMALPRIO+1, game_thd, NULL);
}

