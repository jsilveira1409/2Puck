/*
 * game.c
 *
 *  Created on: 29 Apr 2022
 *      Author: karl
 */

#include <ch.h>
#include <leds.h>
#include <audio_processing.h>
#include <photo.h>
#include "communications.h"
#include "music.h"


/*
 * Messages received and sent to and from the PC
 */
enum{
	// Received messages
	none,
	start_game,
	player1_play,
	player2_play,

	// Sent messages
	chosen_song,
	player1_finish,
	player2_finish,
	send_winner,
	send_photo,
	finished
}message;

static THD_WORKING_AREA(gameWA, 128);

static THD_FUNCTION(game_thd, arg) {

	(void) arg;

	uint8_t msg = none;
	uint8_t score1 = 0;
	uint8_t score2 = 0;

	while (msg != player2_finish) {
		if(msg == none || msg == chosen_song || msg  == player1_finish){
			ReceiveModeFromComputer((BaseSequentialStream *) &SD3, &msg);
			set_body_led(1);
			chThdSleepMilliseconds(100);
			set_body_led(0);
			chThdSleepMilliseconds(100);

		}else if(msg == start_game){
			uint8_t song = random_song();
			SendUint8ToComputer(&song, 1);
			msg = chosen_song;
			set_led(LED1, 1);

		}else if(msg == player1_play){
			/*
			 * Start player 1 recording, compute score and send
			 */
			init_music();
			wait_finish_music();
			score1 = get_score();
			SendUint8ToComputer(&score1, 1);
			msg = player1_finish;
			set_led(LED5,1);

		}else if (msg == player2_play){
			/*
			 * Start player 2 recording, compute score and send
			 */
			wait_finish_music();
			score2 = get_score();
			SendUint8ToComputer(&score2, 1);
			msg = player2_finish;
		}

		chThdSleepMilliseconds(200);
	}
	chThdSleepMilliseconds(2000);
	init_photo();
}

void init_game(void){
	chThdCreateStatic(gameWA, sizeof(gameWA), NORMALPRIO, game_thd, NULL);
}

