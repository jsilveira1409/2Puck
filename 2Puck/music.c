#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <music.h>
#include <audio/audio_thread.h>
#include <audio_processing.h>
#include <audio/microphone.h>
#include <leds.h>
#include <audio/play_melody.h>

#define NB_SONGS 			6
#define MS_IN_MINUTE		(60*1000)		//milliseconds in a minute, needed for bpm to ms conversion
#define SIXTEENTH_NOTE		16

static BSEMAPHORE_DECL(sem_finished_music, TRUE);
/*
 * DATA TYPES AND VARIABLES
 */

static int16_t score = 0;

static uint8_t *recording;

/*
 * The duration unity (1) corresponds to a siteenth note
 * Ex: for bpm = 120 	-> 1 sixteenth = 125ms = 1
 * 						-> 1 quarter = 4 sixteenth = 4
 */

//seven nation army melody by White Stripes
static const uint16_t seven_nation_army_melody[] = {
  0,
  NOTE_D4 , NOTE_D4, 0, NOTE_F4, 0, NOTE_D4, 0, NOTE_C4 , NOTE_AS3, NOTE_A3, 0,
  NOTE_D4 , NOTE_D4, 0, NOTE_F4, 0, NOTE_D4, 0, NOTE_C4 , NOTE_AS3, NOTE_A3, 0,
  NOTE_D4 , NOTE_D4, 0, NOTE_F4, 0, NOTE_D4, 0, NOTE_C4 , NOTE_AS3, NOTE_A3, 0,

};

//seven nation army tempo
static const float seven_nation_army_tempo[] = {
  12,
  3,20,20,20,9,20,9,9,2.2,2.2,70,
  3,20,20,20,9,20,9,9,2.2,2.2,70,
  3,20,20,20,9,20,9,9,2.2,2.2,70,
};


/*
 * Come As you are - Nirvana
 */
static uint8_t melody_COME_AS_YOU_ARE[15] = {
	E1,	E1,	F1,	FS1, A2, FS1, A2, FS1, FS1, F1, E1, B2,	E1, E1, B2
};

static uint8_t duration_COME_AS_YOU_ARE[15] = {
	2, 	2, 	2, 	4, 	2, 	2, 	2, 	2,	2, 	2, 	2, 	2, 	2, 	2, 	2
};

/*
 * Miss You - Rolling Stones
 */
static uint8_t melody_MISS_YOU[18]={
	D1,	E1, G2,	A2, E1, D1,	E1,
	D1,	E1, G2,	A2, E1, D1,	E1,	D1,	E1,	A2,	E1
};

static uint8_t duration_MISS_YOU[18]={
	2,	2, 	2, 	2, 	1,	1, 	6, 	2,	2, 	2, 	2,	1, 	1, 	6,	2,	2,	2,	10
};

/*
 * Killing in the Name of - RATM
 */
static uint8_t melody_KILLING_IN_THE_NAME_OF[20]={
	E1,	C2,	D2, F2, FS2, D2, E1, FS1, G1, FS1,
	E1,	C2,	D2, F2, FS2, D2, E1, FS1, G1, FS1
};

static uint8_t duration_KILLING_IN_THE_NAME_OF[20]={
	4,	2, 	2, 	2, 	2,	2, 	4, 	4,	4, 	4,
	4,	2, 	2, 	2, 	2,	2, 	4, 	4,	4, 	4
};

/*
 * The Man who sold the world - David Bowie
 */

static uint8_t melody_SOLD_THE_WORLD[17]={
	G2, G2, G2, F2, G2, GS2, G2, F2,
	G2, G2, G2, F2, G2, GS2, G2, F2,
	G2
};

static uint8_t duration_SOLD_THE_WORLD[17]={
	2,	2, 	2, 	4, 	1,	1, 	2, 	2,
	2,	2, 	2, 	4, 	1,	1, 	2, 	2,
	15
};


/*
 * Seven Nation Army - Whitesnake
 */

static uint8_t melody_SEVEN_NATION_ARMY[16]={
	E1, E1, G2,	E1,	D1,
	C1,	B1,
	E1, E1, G2,	E1,	D1,
	C1,	D1, C1, B1,

};

static uint8_t duration_SEVEN_NATION_ARMY[16]={
	6,	2,	3,	3,	3,
	8,	8,
	6,	2,	3,	3,	3,
	4, 	4, 	4, 	4,
};
/*
* The Next Episode - Dr Dre
 */

static uint8_t melody_NEXT_EPISODE[24]={
	F3,  X, AS4, X,
	AS4, X, GS4, X, AS4, X,
	GS4, X, FS4, X, GS4, X,
	GS4, X, FS4, X, F3,  X, FS4, X
};

static uint8_t duration_NEXT_EPISODE[24]={
	6, 2, 6, 2,
	4, 2, 4, 2, 4, 2,
	4, 2, 4, 2, 4, 2,
	4, 2, 4, 2, 4, 2, 4, 2
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
	uint16_t melody_size;
}songs[NB_SONGS] = {
		{melody_COME_AS_YOU_ARE,			duration_COME_AS_YOU_ARE,			50, 	15},
		{melody_MISS_YOU,					duration_MISS_YOU,					50,		18},
		{melody_KILLING_IN_THE_NAME_OF, 	duration_KILLING_IN_THE_NAME_OF,	50,		20},
		{melody_SOLD_THE_WORLD, 			duration_SOLD_THE_WORLD,			50,		17},
		{melody_SEVEN_NATION_ARMY,			duration_SEVEN_NATION_ARMY,			50,		16},
		{melody_NEXT_EPISODE,				duration_NEXT_EPISODE,				50,		24}
};



/*
 * Static Functions
 */

static THD_WORKING_AREA(musicWorkingArea, 128);
static THD_FUNCTION(music, arg) {

	(void) arg;

  while (true) {

	  chThdSleepMilliseconds(2000);
//	  wait_finish_playing();
//	  set_recording(get_recording());
//	  score += check_note_sequence(come_as_you_are);
//	  score += check_note_order(come_as_you_are);
//	  set_led(LED1, 0);
//	  chBSemSignal(&sem_finished_music);
//	  set_led(LED5, 0);
//	  chThdSleepMilliseconds(2000);
  }
}

/*
 * FUNCTIONS
 */

void wait_finish_music(void){
	chBSemWait(&sem_finished_music);
}

uint8_t random_song(void){
	return 2;
}

/*
 * Checking notes time sequence is correct: was note x played when it should
 * be played ?
 */

int16_t check_note_sequence(uint8_t index){
	int16_t score = 0;
	uint16_t starting_index = 0;
	/*
	 * First we find the first correct note on the recording,
	 * which will be our starting index for the melody-recording
	 * comparison
	 */
	for(uint16_t i=0; i<songs[index].melody_size; i++){
		if(((songs[index].melody_ptr[i])%12) == recording[i]){
			starting_index = i;
			break;
		}
	}
	/*
	 * Here we compare the melody and the recording
	 */
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

/*
 * Threads
 */


/*
 * Public Functions
 */

void init_music(void){
    mic_start(&processAudioDataCmplx);
	chThdCreateStatic(musicWorkingArea, sizeof(musicWorkingArea),
			NORMALPRIO, music, NULL);
}

void play_song(uint8_t index){
	for(uint8_t i = 0; i < songs[index].melody_size; i++){
		if(note_frequency[songs[index].melody_ptr[i]] == X){		//Silence
			dac_stop();
		}else{
			dac_play(note_frequency[songs[index].melody_ptr[i]]);
		}
		chThdSleepMilliseconds(duration_to_ms(songs[index].note_duration_ptr[i],songs[index].bpm));
	}
}


void set_recording(uint8_t *data){
	recording = data;
}


int16_t get_score(void){
	return score;
}
