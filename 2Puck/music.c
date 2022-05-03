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
#include <sdio.h>
#include <fat.h>
#include <audio/play_sound_file.h>


#define NB_SONGS 						6
#define COME_AS_YOU_ARE_SIZE			15
#define MISS_YOU_SIZE					18
#define SOLD_THE_WORLD_SIZE				17
#define KILLING_IN_THE_NAME_OF_SIZE		20
#define SEVEN_NATION_SIZE				16
#define NEXT_EPISODE_SIZE				12

static BSEMAPHORE_DECL(sem_finished_music, TRUE);
static int16_t score = 0;
static uint8_t *recording;


/*
 * Come As you are - Nirvana
 */
static uint8_t melody_COME_AS_YOU_ARE [COME_AS_YOU_ARE_SIZE] = {
	E1,	E1,	F1,	FS1, A2, FS1, A2, FS1, FS1, F1, E1, B2,	E1, E1, B2
};

/*
 * Miss You - Rolling Stones
 */
static uint8_t melody_MISS_YOU [MISS_YOU_SIZE] = {
	D1,	E1, G2,	A2, E1, D1,	E1,
	D1,	E1, G2,	A2, E1, D1,	E1,	D1,	E1,	A2,	E1
};

/*
 * Killing in the Name of - RATM
 */
static uint8_t melody_KILLING_IN_THE_NAME_OF [KILLING_IN_THE_NAME_OF_SIZE] = {
	E1,	C2,	D2, F2, FS2, D2, E1, FS1, G1, FS1,
	E1,	C2,	D2, F2, FS2, D2, E1, FS1, G1, FS1
};

/*
 * The Man who sold the world - David Bowie
 */
static uint8_t melody_SOLD_THE_WORLD [SOLD_THE_WORLD_SIZE] = {
	G2, G2, G2, F2, G2, GS2, G2, F2,
	G2, G2, G2, F2, G2, GS2, G2, F2,
	G2
};

/*
 * Seven Nation Army - Whitesnake
 */
static uint8_t melody_SEVEN_NATION_ARMY [SEVEN_NATION_SIZE] = {
	E1, E1, G2,	E1,	D1,
	C1,	B1,
	E1, E1, G2,	E1,	D1,
	C1,	D1, C1, B1,
};

/*
* The Next Episode - Dr Dre
 */
static uint8_t melody_NEXT_EPISODE [NEXT_EPISODE_SIZE] = {
	F3, AS4,
	AS4,GS4,AS4,
	GS4,FS4,GS4,
	GS4, FS4,F3, FS4
};


/*
 * Song data type
 * Contains the melody, the corresponding note duration where 1 = sixteenth note (double crochet)
 * and the melody size
 */
struct song{
	uint8_t * melody_ptr;
	uint16_t melody_size;
	char* file_name;
}songs[NB_SONGS] = {
		{melody_COME_AS_YOU_ARE,			COME_AS_YOU_ARE_SIZE,			"asyouare.wav"},
		{melody_MISS_YOU,					MISS_YOU_SIZE		,			"missyou.wav"},
		{melody_KILLING_IN_THE_NAME_OF, 	KILLING_IN_THE_NAME_OF_SIZE,	"killingin.wav"},
		{melody_SOLD_THE_WORLD, 			SOLD_THE_WORLD_SIZE,			"soldtheworld.wav"},
		{melody_SEVEN_NATION_ARMY,			SEVEN_NATION_SIZE,				"sevennation.wav"},
		{melody_NEXT_EPISODE,				NEXT_EPISODE_SIZE,				"nextepisode.wav"}
};



/*
 * Static Functions
 */
/*
 * Checking notes time sequence is correct: was note x played when it should
 * be played ?
 */
static int16_t check_note_sequence(uint8_t index){
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
static int16_t check_note_order(uint8_t index){
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
 * THREADS
 */

static THD_WORKING_AREA(musicWorkingArea, 128);
static THD_FUNCTION(music_thd, arg) {

	(void) arg;

	while (true) {

		wait_finish_playing();
		set_recording(get_recording());
		score += check_note_sequence(COME_AS_YOU_ARE);
		score += check_note_order(COME_AS_YOU_ARE);
		set_led(LED1, 0);
		chBSemSignal(&sem_finished_music);
		set_led(LED5, 0);
		chThdSleepMilliseconds(1000);
	}
}


/*
 * Public Functions
 */

void init_music(void){
//    mic_start(&processAudioDataCmplx);
    playSoundFileStart();
    sdio_start();
    dac_start();

    while(!mountSDCard()){
		set_body_led(1);
		chThdSleepMilliseconds(200);
		set_body_led(0);
		chThdSleepMilliseconds(200);
	}

//	chThdCreateStatic(musicWorkingArea, sizeof(musicWorkingArea),
//			NORMALPRIO, music_thd, NULL);
}

void play_song(jukebox index){
	setSoundFileVolume(30);
	playSoundFile(songs[index].file_name, SF_FORCE_CHANGE);
//	waitSoundFileHasFinished();

}

void stop_song(void){
	stopCurrentSoundFile();
}


void set_recording(uint8_t *data){
	recording = data;
}

int16_t get_score(void){
	return score;
}

void wait_finish_music(void){
	chBSemWait(&sem_finished_music);
}


uint8_t random_song(void){
	return 2;
}
