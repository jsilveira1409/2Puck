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
#include "pathing.h"


typedef enum {
	IDLE,
	START_GAME,
	GOTO_WINNER,
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
				music_init();
				song = get_song();
				SendUint8ToComputer(&song, 1);
				set_led(LED1, 1);

//				pathing_set(DANCE);
				wait_finish_music();
				pathing_set(WAIT);
				score1 = get_score();
				SendUint8ToComputer(&score1, 1);
				set_led(LED5,1);

//				pathing_set(DANCE);
				wait_finish_music();
				pathing_set(WAIT);
				score2 = get_score();
				SendUint8ToComputer(&score2, 1);
				music_stop();
				state++;
				break;

			case GOTO_WINNER:
				set_body_led(0);
				pathing_set((score1 >= score2) ? PATH_TO_PLAYER1 : PATH_TO_PLAYER2);
				pathing_wait_finish();
				state++;
				break;

			case SEND_PHOTO:
				set_body_led(0);
				init_photo();
				state++;
				break;

			case FINISHED:
				state = IDLE;
				break;
		}
		chThdSleepMilliseconds(300);
	}
}

void game_init(void){
	chThdCreateStatic(gameWA, sizeof(gameWA), NORMALPRIO+1, game_thd, NULL);
}

