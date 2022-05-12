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
#include <chprintf.h>
#include "communications.h"
#include "music.h"
#include "pathing.h"
#include "lightshow.h"

static thread_t* gameThd = NULL;

typedef enum {
	IDLE,
	START_GAME,
	GOTO_WINNER,
	SEND_PHOTO,
	FINISHED
} GAME_STATE;

static float get_score(void){
	/* Waiting for a queued message then retrieving it.*/
	thread_t *tp = chMsgWait();
	float freq = (float)chMsgGet(tp);

	/* Sending back an acknowledge.*/
	chMsgRelease(tp, MSG_OK);

	return freq;
}

static THD_WORKING_AREA(gameWA, 128);

static THD_FUNCTION(game_thd, arg) {

	(void) arg;

	GAME_STATE state = IDLE;
	uint8_t message = 0;
	song_selection_t song = 0;
	uint8_t recording_size = 20;
	uint8_t num_players = 2;
	uint8_t score[num_players]; //TODO: SHOULD BE A FLOAT

	while(true) {
		switch(state){
			case IDLE:
				message = chSequentialStreamGet(&SD3);
				if (message == 'w'){
					state++;
				}
				break;

			case START_GAME:
				song = music_init();
				chThdSleepMilliseconds(400);
//				pathing_init();
//				lightshow_init();
				SendUint8ToComputer(&song, 1);

				for(uint8_t i=0; i<num_players; i++){
					set_led(LED5, 1);
					music_listen(recording_size);
					score[i] = get_score();
					SendUint8ToComputer(&score[i], 1);
					chThdSleepMilliseconds(1000);
					set_led(LED1, 1);
				}

				music_stop();
				set_body_led(1);
				state++;
				break;

			case GOTO_WINNER:
#ifdef	PLAY_SONGS
				play_song(NEXT_EPISODE);
#endif
				chThdSleepMilliseconds(5000);
				pathing_init((score[0] >= score[1]) ? PATH_TO_PLAYER1 : PATH_TO_PLAYER2);
				pathing_wait_finish();
				pathing_stop();
#ifdef PLAY_SONGS
				stop_song();
#endif
				state++;
				break;

			case SEND_PHOTO:
				photo_init();
				photo_wait_finish();
				photo_stop();
				chThdSleepMilliseconds(10000);
				state++;
				break;

			case FINISHED:
				state = IDLE;
				set_body_led(1);
				break;
		}
		chThdSleepMilliseconds(300);
	}
}

void game_init(void){
	gameThd = chThdCreateStatic(gameWA, sizeof(gameWA), NORMALPRIO+1, game_thd, NULL);
}

msg_t game_send_score(float score){
	return chMsgSend(gameThd, (msg_t)score);
}
