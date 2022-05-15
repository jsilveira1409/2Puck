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
#include <chprintf.h>
#include "music.h"
#include "pathing.h"
#include "lightshow.h"
#include "console.h"

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
	float score = (float)chMsgGet(tp);

	/* Sending back an acknowledge.*/
	chMsgRelease(tp, MSG_OK);

	return score;
}

static THD_WORKING_AREA(gameWA, 512);

static THD_FUNCTION(game_thd, arg) {

	(void) arg;

	GAME_STATE state = IDLE;
	song_selection_t song = 0;
	uint8_t recording_size = 18;
	uint8_t num_players = 2;
	int16_t score[num_players];

	while(true) {
		switch(state){
			case IDLE:
				;
				char c = console_get_char("Send w to start the Game");
				if (c == 'w'){
					state++;
				}
				break;

			case START_GAME:
				console_send_string("Game Started");
				song = music_init();
				const char* music_name = music_song_name(song);
				console_send_string("The song chosen is");
				console_send_string(music_name);
//				lightshow_init();

				for(uint8_t i=0; i<num_players; i++){
					chThdSleepMilliseconds(4000);
					console_send_string("Player Start");
					set_led(LED5, 1);
					music_listen(recording_size);
					score[i] = get_score();
					console_send_string("Calculating Score...");
					console_send_int(score[i],"Your score is");
					console_send_string("Player finished");
					set_led(LED1, 1);
				}
				set_body_led(1);
				music_stop();
				state++;
				break;

			case GOTO_WINNER:
				console_send_string("Going to Winner");
				play_song(song);
				pathing_init((score[0] >= score[1]) ? PATH_TO_PLAYER1 : PATH_TO_PLAYER2);
				pathing_wait_finish();
				state++;
				break;

			case SEND_PHOTO:
				console_send_string("Taking Photo");
				photo_init();
				photo_wait_finish();
				photo_stop();
				console_send_string("Photo taken");
				state++;
				break;

			case FINISHED:
				stop_song();
				console_send_string("Game finished");
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
