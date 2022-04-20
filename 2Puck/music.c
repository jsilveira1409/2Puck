#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <music.h>
#include <audio/audio_thread.h>
#include <audio_processing.h>

#define NB_SONGS 4


/*
 * DATA TYPES AND VARIABLES
 */

/*
 * Enum for others functions to choose the song more explicitely,
 * Basically the song index in the songs array
 */
enum jukebox{come_as_you_are, miss_you, killing_in_the_name_of, sold_the_world};
enum chromatic_scale{
	A1, AS1, B1, C1, CS1, D1, DS1,E1, F1, FS1, G1, GS1,
	A2, AS2, B2, C2, CS2, D2, DS2,E2, F2, FS2, G2, GS2
};

static uint8_t *recording;

/*
 * Come As you are - Nirvana
 */
static uint8_t melody_COME_AS_YOU_ARE[15] = {
	E1,	E1,	F1,	FS1, A2, FS1, A2, FS1, FS1, F1, E1, B2,	E1, E1, B2
};

static uint8_t duration_COME_AS_YOU_ARE[15] = {
	1, 	1, 	1, 	1, 	1, 	1, 	1, 	1,	1, 	1, 	1, 	1, 	1, 	1, 	1
};

/*
 * Miss You - Rolling Stones
 */
static uint8_t melody_MISS_YOU[18]={
	D1,	E1, G1,	A1, E1, D1,	E1, D1,	E1, G1,	A1, E1, D1,	E1,	D1,	E1,	A1,	E1
};

static uint8_t duration_MISS_YOU[18]={
	1,	1, 	1, 	1, 	1,	1, 	1, 	1,	1, 	1, 	1,	1, 	1, 	1,	1,	1,	1,	1
};

/*
 * Killing in the Name of - RATM
 */
static uint8_t melody_KILLING_IN_THE_NAME_OF[10]={
	E1,	C1,	D1, F2, FS2, D1, E1, FS1, G1, FS1
};

static uint8_t duration_KILLING_IN_THE_NAME_OF[10]={
	1,	1, 	1, 	1, 	1,	1, 	1, 	1,	1, 	1
};

/*
 * The Man who sold the world - David Bowie
 */

static uint8_t melody_SOLD_THE_WORLD[17]={
	A2, A2, A2, G2, A2, AS2, A2, G2,
	A2, A2, A2, G2, A2, AS2, A2, G2,
	A2,
};

static uint8_t duration_SOLD_THE_WORLD[17]={
	2,	2, 	2, 	2, 	1,	1, 	1, 	2,
	2,	2, 	2, 	2, 	1,	1, 	1, 	2,
	5
};

/*
 * Song data type
 * Contains the melody, the corresponding note duration where 1 = sixteenth note (double crochet)
 * and the melody size
 */


struct song{
	uint8_t * melody_ptr;
	uint8_t * note_duration_ptr;
	uint16_t bpm;
	uint8_t melody_size;
}songs[NB_SONGS] = {
		{&melody_COME_AS_YOU_ARE,			&duration_COME_AS_YOU_ARE,			100, 	15},
		{&melody_MISS_YOU,					&duration_MISS_YOU,					100,	18},
		{&melody_KILLING_IN_THE_NAME_OF, 	&duration_KILLING_IN_THE_NAME_OF,	100,	10},
		{&melody_SOLD_THE_WORLD, 			&duration_SOLD_THE_WORLD,			100,	17}
};




/*
 * THREADS
 */

static THD_WORKING_AREA(musicWorkingArea, 128);



static THD_FUNCTION(music, arg) {

  while (true) {
	  play_song(come_as_you_are);
	  chThdSleepMilliseconds(2000);
	  play_song(miss_you);
	  chThdSleepMilliseconds(2000);
	  play_song(killing_in_the_name_of);
	  chThdSleepMilliseconds(2000);
	  play_song(sold_the_world);
	  chThdSleepMilliseconds(2000);
  }
}



/*
 * FUNCTIONS
 */

void init_music(){
	chThdCreateStatic(musicWorkingArea, sizeof(musicWorkingArea),
	                             NORMALPRIO, music, NULL);
	dac_start();

}


void play_song(uint8_t index){
	for(uint8_t i = 0; i < songs[index].melody_size; i++){
		dac_play(note_frequency[songs[index].melody_ptr[i]]);

		chThdSleepMilliseconds(100 * songs[index].note_duration_ptr[i]);
		dac_stop();
		chThdSleepMilliseconds(100);
	}

}





/*
 * Checking notes time sequence is correct: was note x played when it should
 * be played ?
 */

int16_t check_note_sequence(uint8_t index){
	int16_t score = 0;
	uint16_t starting_index = 0;

	for(uint16_t i=0; i<songs[index].melody_size; i++){
		if((songs[index].melody_ptr[i]%12) == recording[i]){
			starting_index = i;
			break;
		}
	}
	for(uint16_t i=starting_index; i< (starting_index+songs[index].melody_size); i++){
		if(recording[i] == (songs[index].melody_ptr[i]%12)){
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
int16_t check_note_order(uint8_t index){
	int16_t score = 0;

	for(uint16_t i=0; i< songs[index].melody_size; i++){
		for(uint16_t j=i; j < songs[index].melody_size; j++){
			if((songs[index].melody_ptr[i]%12) == recording[j]){
				score ++;
				break;
			}
		}
	}
	return score;
}


void set_recording(uint8_t *data){
	recording = data;
}

