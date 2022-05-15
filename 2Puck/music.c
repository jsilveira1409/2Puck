#include <ch.h>
#include <hal.h>
#include <sdio.h>
#include <fat.h>
#include <stdlib.h>
#include <audio/audio_thread.h>
#include <audio_processing.h>
#include <audio/microphone.h>
#include <audio/play_sound_file.h>
#include "rng.h"
#include <leds.h>
#include "music.h"
#include "game.h"
#include "console.h"

#define POSITIVE_POINTS 		4
#define NEGATIVE_POINTS			1
#define MAX_ACCEPTABLE_FREQ_ERROR	7

typedef enum {
	A1=0, AS1, B1, C1, CS1, D1, DS1,E1, F1, FS1, G1, GS1,
	A2,   AS2, B2, C2, CS2, D2, DS2,E2, F2, FS2, G2, GS2,
	A3,   AS3, B3, C3, CS3, D3, DS3,E3, F3, FS3, G3, GS3,
	A4,   AS4, B4, C4, CS4, D4, DS4,E4, F4, FS4, G4, GS4,
	A5,   AS5, B5, C5, CS5, D5, DS5,E5, F5, FS5, G5, GS5,
	NONE
}note_t;

typedef struct{
	char* name;
	uint16_t freq;
}note_struct_t;

note_struct_t notes[] = {
		{"A1",55}, {"AS1",58}, {"B1",62}, {"C1",65},  {"CS1",69},  {"D1",73},
		{"DS1",77},  {"E1",82},  {"F1",87},  {"FS1",92},  {"G1",98},  {"GS1",104},

		{"A2",110},{"AS2",116},{"B2",124},{"C2",131}, {"CS2",138}, {"D2",146},
		{"DS2",155}, {"E2",165}, {"F2",175}, {"FS2",185}, {"G2",196}, {"GS2",208},

		{"A3",220},{"AS3",233},{"B3",247},{"C3",262}, {"CS3",277}, {"D3",294},
		{"DS3",311}, {"E3",330}, {"F3",349}, {"FS3",370}, {"G3",392}, {"GS3",415},

		{"A4",440},{"AS4",466},{"B4",494},{"C4",523}, {"CS4",554}, {"D4",587},
		{"DS4",622}, {"E4",659}, {"F4",698}, {"FS4",740}, {"G4",784}, {"GS4",831},

		{"A5",880},{"AS5",932},{"B5",988},{"C5",1047},{"CS5",1108},{"D5",1174},
		{"DS5",1244},{"E5",1318},{"F5",1396},{"FS5",1480},{"G5",1568},{"GS5",1662},
};

static const char* song_name[] = {
		[COME_AS_YOU_ARE]	= "Come as you are ~Nirvana",
		[MISS_YOU]		  	= "Miss you ~Rolling Stones",
		[SOLD_THE_WORLD]  	= "The Man Who Sold the World ~Nirvana",
		[SEVEN_NATION] 	  	= "Seven Nation Army ~White Snakes",
		[NEXT_EPISODE]		= "The Next Episode ~Dr Dre",
};


/*
 * Come As you are - Nirvana
 */
static const note_t melody_COME_AS_YOU_ARE[] = {
	E1,	E1,	F1,	FS1, A2, FS1, A2, FS1, FS1, F1, E1, B2,	E1, E1, B2
};

/*
 * Miss You - Rolling Stones
 */
static const note_t melody_MISS_YOU [] = {
	D1,	E1, G2,	A2, E1, D1,	E1,
	D1,	E1, G2,	A2, E1, D1,	E1,
	D1,	E1,	A2,	E1
};

/*
 * The Man who sold the world - David Bowie
 */
static const note_t melody_SOLD_THE_WORLD[] = {
	G2, G2, G2, F2, G2, GS2, G2, F2,
	G2, G2, G2, F2, G2, GS2, G2, F2,
	G2
};

/*
 * Seven Nation Army - Whitesnake
 */
static const note_t melody_SEVEN_NATION_ARMY[] = {
	E1, E1, G2,	E1,	D1,
	C1,	B1,
	E1, E1, G2,	E1,	D1,
	C1,	D1, C1, B1,
};

/*
* The Next Episode - Dr Dre
 */
static const note_t melody_NEXT_EPISODE[] = {
	F3, AS4, AS4, GS4, AS4, GS4, FS4, GS4, GS4, FS4, F3, FS4
};

/*
 * Song data type
 * Contains the melody, the corresponding note duration where 1 = sixteenth note (double crochet)
 * and the melody size
 */
typedef struct{
	const uint8_t* melody_ptr;
	const uint16_t melody_size;
	char* file_name;
}song;

const song songs[] = {
		{melody_COME_AS_YOU_ARE,	sizeof(melody_COME_AS_YOU_ARE),		"asyouare.wav"},
		{melody_MISS_YOU,			sizeof(melody_MISS_YOU)		,		"missyou.wav"},
		{melody_SOLD_THE_WORLD, 	sizeof(melody_SOLD_THE_WORLD),		"soldtheworld.wav"},
		{melody_SEVEN_NATION_ARMY,	sizeof(melody_SEVEN_NATION_ARMY),	"sevennation.wav"},
		{melody_NEXT_EPISODE,		sizeof(melody_NEXT_EPISODE),		"nextepisode.wav"}
};

static thread_t* musicThd = NULL;
static thread_reference_t musicThdRef = NULL;
static song_selection_t chosen_song = 0;
// TODO: CHECK IF NECESSARY AS GLOBAL
static note_t played_notes[50];

/*
 * Static Functions
 */
/*
 * @brief checks notes time sequence is correct: was note x played when it should
 * be played ?
 * @param[in] song_selectrion_t song_index: index of the song in the songs array
 * @return int16_t: total score of the recording
 */
static int16_t check_note_sequence(song_selection_t song_index){
	int16_t points = 0;

	for(uint16_t i=0; i< songs[song_index].melody_size; i++){
		if((played_notes[i]%12) == (((uint8_t)songs[song_index].melody_ptr[i]) % 12)){
			points += POSITIVE_POINTS;
		}else{
			points -= NEGATIVE_POINTS;
		}
	}
	return points;
}

/*
 * @brief calculates the score percentage, from -100% to 100%, of the
 * recording compared to the song's melody
 * @return int16_t : score percentage
 */
static int16_t calculate_score(void){
	int16_t total_score = 0;
	total_score = (int16_t)(100*(float)check_note_sequence(chosen_song) /
			(POSITIVE_POINTS * (float)songs[chosen_song].melody_size));
	if(total_score > 100){
		total_score = 100;
	}else if(total_score < -100){
		total_score = -100;
	}
	return total_score;
}

/*
 * @brief waits for the message from audio_processing.c with the given
 * frequency of the fundamental note played
 * @return float frequency
 */

static float get_frequency(void){
	/* Waiting for a queued message then retrieving it.*/
	thread_t *tp = chMsgWait();
	float freq = (float)chMsgGet(tp);

	/* Sending back an acknowledge.*/
	chMsgRelease(tp, MSG_OK);

	return freq;
}

/*
 * @brief Finds the smallest error between the FFT data
 * and the discrete note frequency in note_frequency[]
 * @param[in] float freq: frequency of the note
 * @return note_t chromatic scale note
 *
 */

static note_t freq_to_note(float freq){
	float smallest_error = MAX_ACCEPTABLE_FREQ_ERROR;
	float curr_error     = 0;
	note_t note = NONE;

	for(uint8_t i = 0; i < NB_NOTES; i++){
		curr_error = abs(freq - (float)notes[i].freq);
		if(curr_error < smallest_error){
			smallest_error = curr_error;
			//uint16_t discret_freq = note_freq[i];
			note = i;
		}
	}
	return note;
}

/*
 * THREADS
 */

static THD_WORKING_AREA(musicWorkingArea, 512);
static THD_FUNCTION(music, arg) {

	(void) arg;

	uint8_t recording_size = 0;

	while(!chThdShouldTerminateX()) {
		//this thread is waiting until it receives a message
		chSysLock();
		recording_size = chThdSuspendS(&musicThdRef);
		chSysUnlock();
		int16_t score = 0;

		for(uint8_t i=0; i<recording_size; i++){
			set_led(LED3, 1);
			float freq = get_frequency();
			note_t note = freq_to_note(freq);
			if(note != NONE){
				played_notes[i] = note;
				console_send_string(notes[note].name);
				set_led(LED3,0);
			}else{
				i--;
			}
		}

		score = calculate_score();
		game_send_score(score);
		chThdSleepMilliseconds(500);
	}
	chThdExit(MSG_OK);
}


/*
 * Public Functions
 */

/*
 * @brief initializes the microphone thread, with the callback function
 * from audio_processing, initializes the music thread and the DAC for
 * the buzzer. Also, chooses the random song.
 * @return (song_selection_t) random song
 */
song_selection_t music_init(void){
	chosen_song = choose_random_song();
	mic_start(&processAudioDataCmplx);
    musicThd = chThdCreateStatic(musicWorkingArea, sizeof(musicWorkingArea),
			NORMALPRIO, music, NULL);
    dac_start();
    return chosen_song;
}

/*
 * @brief stop the microphone and the music thread
 */

void music_stop(void){
	//TODO: Stop TIM9
	mp45dt02Shutdown();
	chThdTerminate(musicThd);
}

/*
 * @brief
 * @param[in] uint8_t recording_size:
 */
void music_listen(uint8_t recording_size){
	 chThdResume(&musicThdRef, (msg_t)recording_size);
}

/*
 *@brief sends the name of the file to play to the play_sound_file.h lib
 *@param[in] song_selection_t index: index of the song in songs to play
 */
void play_song(song_selection_t index){
	sdio_start();
	playSoundFileStart();
	if(!mountSDCard()){
		console_send_string("SD Card not Mounted");
		return;
	}
	setSoundFileVolume(50);
	playSoundFile(songs[index].file_name, SF_FORCE_CHANGE);
}

/*
 * @brief stops the current song, played with the buzzer, started
 * with play_song()
 */
void stop_song(void){
	stopCurrentSoundFile();
}

/*
 * @brief chooses a random song using the rng.h lib and
 * modulating by the number of songs
 *
 * @return song_selection_t index of the random song
 */
song_selection_t choose_random_song(void){
	rng_init();
	uint32_t random = rng_get();
	uint32_t songs_size = sizeof(songs)/sizeof(song);
	uint32_t song_nb = (random % songs_size);
	rng_stop();
	return song_nb;
}

msg_t music_send_freq(float freq){
	return chMsgSend(musicThd, (msg_t)freq);
}

const char* music_song_name(song_selection_t song){
	return song_name[song];
}
