/*
 * game.c
 *
 *  Created on: 29 Apr 2022
 *  Authors: Karl Khalil
 *  		 Joaquim Silveira
 */

//Comment this line to not play the songs
//#define PLAY_SONGS
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
					state++;
				}
				break;

			case START_GAME:
				music_init();
				pathing_init();
				song = get_song();
				SendUint8ToComputer(&song, 1);
				wait_finish_music();
				score1 = get_score();
				SendUint8ToComputer(&score1, 1);
				chThdSleepMilliseconds(2000);
				wait_finish_music();
				score2 = get_score();
				SendUint8ToComputer(&score2, 1);
				music_stop();
				chThdSleepMilliseconds(2000);
				state++;
				break;

			case GOTO_WINNER:
#ifdef	PLAY_SONGS
				play_song(NEXT_EPISODE);
#endif
				pathing_set((score1 >= score2) ? PATH_TO_PLAYER1 : PATH_TO_PLAYER2);
				pathing_wait_finish();
				pathing_stop();
#ifdef PLAY_SONGS
				stop_song();
#endif
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
//		chThdSleepMilliseconds(300);
	}
}

void game_init(void){
	chThdCreateStatic(gameWA, sizeof(gameWA), NORMALPRIO+1, game_thd, NULL);
}

