#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <music.h>

#define NB_SONGS 	2

enum jukebox{come_as_you_are, miss_you};

static uint8_t *recording;

static const uint8_t COME_AS_YOU_ARE[15] = {
/*
 * 	E	E	F	F#	A	F#	A	F#	F#	F	E	B	E	E	B
 */
	7,	7, 	8, 	9, 	0, 	9, 	0, 	9, 	9, 	8, 	7, 	2, 	7, 	7, 	2
};

static const uint8_t MISS_YOU[18]={
/*
 *  D	E	G	A	E	D	E	D	E	G	A	E	D	E	D	E	A	E
 */
	5,	7,	10,	0,	7,	5,	7,	5,	7,	10,	0,	7,	5,	7,	5,	7,	0,	7
};


static const uint8_t *song_list[NB_SONGS] ={COME_AS_YOU_ARE, MISS_YOU};


/*
 * Checking notes time sequence is correct: was note x played when it should
 * be played ?
 */

int16_t check_note_sequence(uint8_t song_index){
	uint16_t melody_size = sizeof(*song_list[song_index]);
	int16_t score = 0;
	for(uint16_t i=0; i< melody_size; i++){
		if(recording[i] == (song_list[song_index])[i]){
			score ++;
		}else{
			score --;
		}
	}
	return score;
}


/*
 * Checking order of played notes is correct: was note y played after note x, even
 * if there is a wrong note in between?
 */
int16_t check_note_order(uint8_t song_index){
	uint16_t melody_size = sizeof(*song_list[song_index]);
	int16_t score = 0;

	for(uint16_t i=0; i< melody_size; i++){
		for(uint16_t j=i; j < melody_size; j++){
			if((song_list[song_index])[i] == recording[j]){
				score ++;
				break;
			}else{
				score --;
			}
		}
	}
	return score;
}
