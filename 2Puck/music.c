#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <music.h>
#include <audio/audio_thread.h>
#include <audio_processing.h>
#include <audio/microphone.h>
#include <sdio.h>
#include <fat.h>
#include <audio/play_sound_file.h>
#include <rng.h>

#define DEBUG_SCORING_ALGO

#define NB_SONGS 						6
#define COME_AS_YOU_ARE_SIZE			15
#define MISS_YOU_SIZE					18
#define SOLD_THE_WORLD_SIZE				17
#define KILLING_IN_THE_NAME_OF_SIZE		20
#define SEVEN_NATION_SIZE				16
#define NEXT_EPISODE_SIZE				12

static BSEMAPHORE_DECL(sem_finished_music, TRUE);
static float score = 0;
static uint8_t* recording = NULL;

static thread_t* musicThd = NULL;

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
	D1,	E1, G2,	A2, E1, D1,	E1,
	D1,	E1,	A2,	E1
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
	F3, AS4, AS4,GS4,AS4,GS4,FS4,GS4,GS4, FS4,F3, FS4
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

static song_selection chosen_song = 0;

/*
 * Static Functions
 */

static void shift_to_correct_note(song_selection song_index, uint32_t starting_index, uint16_t *next_correct_index){
	for(uint16_t i=starting_index; i<songs[song_index].melody_size; i++){
		if(((songs[song_index].melody_ptr[i])%12) == recording[i]){
			*next_correct_index = i;
			break;
		}
	}
}

/*
 * Checking notes time sequence is correct: was note x played when it should
 * be played ?
 */
static int16_t check_note_sequence(song_selection song_index){
	 int16_t points = 0;
	 uint16_t correct_index = 0;
	 uint16_t note_index = 0;
	/*
	 * First we find the first correct note on the recording,
	 * which will be our starting index for the melody-recording
	 * comparison
	 */
	shift_to_correct_note(song_index, note_index, &correct_index);
	/*
	 * Here we compare the melody and the recording
	 */
	for(uint16_t i=correct_index; i< (correct_index + songs[song_index].melody_size); i++){
		if((recording[i]%12) == (((uint8_t)songs[song_index].melody_ptr[note_index]) % 12)){
			points ++;
			note_index ++;
		}else{
			points --;
		}
	}
	return points;
}


/*
 * Checking order of played notes is correct: was note y played after note x, even
 * if there is a wrong note in between?
 */
static int16_t check_note_order(song_selection song_index){
	 int16_t points = 0;
	 uint16_t shift = 0;

	for(uint16_t i = 0; i < songs[song_index].melody_size; i++){
		for(uint16_t j = i+shift; j < songs[song_index].melody_size; j++){
			if((songs[song_index].melody_ptr[i]%12) == (recording[j]%12)){
				points ++;
				break;
			}else{
				shift ++;
				points --;
			}
		}
	}
	return points;
}

static float calculate_score(song_selection song_index){
	 float total_score = 0;
	 float percentage = 0;

	total_score = check_note_sequence(song_index) + check_note_order(song_index);
//	percentage = 100*((float)total_score/(float)(songs[song_index].melody_size*2));
//	if(percentage < 0){
//		percentage = 0;
//	}
	return total_score;
}
/*
 * THREADS
 */

static THD_WORKING_AREA(musicWorkingArea, 128);
static THD_FUNCTION(music, arg) {

	(void) arg;

	chosen_song = random_song();
	 uint8_t data [15] = {E1,	E1,	F1,	FS1, A2, FS1, A2, FS1, FS1, F1, E1, B2,	E1, E1, B2};

	while (!chThdShouldTerminateX()) {
		score = 0;
		wait_finish_playing();
		set_recording(get_recording());
		score = calculate_score(chosen_song);
		chBSemSignal(&sem_finished_music);
		chThdSleepMilliseconds(500);
	}
	chThdExit(0);
}


/*
 * Public Functions
 */
void music_init(void){
	chosen_song = random_song();
	mic_start(&processAudioDataCmplx);
    musicThd = chThdCreateStatic(musicWorkingArea, sizeof(musicWorkingArea),
			NORMALPRIO, music, NULL);
    dac_start();
}

void music_stop(void){
	// TODO: Stop TIM9
	mp45dt02Shutdown();
	chThdTerminate(musicThd);
}

void play_song(song_selection index){
	setSoundFileVolume(20);
	playSoundFile(songs[index].file_name, SF_FORCE_CHANGE);
//	waitSoundFileHasFinished();  --> blocks the motors, logical

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

song_selection get_song(void){
	rng_init();
	uint32_t random_val = (rng_get() % NB_SONGS);
	rng_stop();
	return random_val;
}

void wait_finish_music(void){
	chBSemWait(&sem_finished_music);
}

uint8_t random_song(void){
	rng_init();
	chosen_song = (rng_get() % NB_SONGS);
	rng_stop();
	return chosen_song;
}


