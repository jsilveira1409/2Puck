#ifndef MUSIC_H
#define MUSIC_H


typedef enum {
	COME_AS_YOU_ARE,
	MISS_YOU,
	KILLING_IN_THE_NAME_OF,
	SOLD_THE_WORLD
}song_selection;

/*
 * Enum for others functions to choose the song more explicitely,
 * Basically the song index in the songs array
 */
typedef enum {
	A1, AS1, B1, C1, CS1, D1, DS1,E1, F1, FS1, G1, GS1,
	A2, AS2, B2, C2, CS2, D2, DS2,E2, F2, FS2, G2, GS2,
	A3, AS3, B3, C3, CS3, D3, DS3,E3, F3, FS3, G3, GS3
}chromatic_scale;

/*
 * Public Functions
 */
void init_music(void);
void play_song(uint8_t index);
void set_recording(uint8_t *data);
uint8_t random_song(void);
void wait_finish_music(void);
int16_t get_score(void);
song_selection get_song(void);
/*
 * Static functions
 */
int16_t check_note_order(uint8_t index);
int16_t check_note_sequence(uint8_t index);
#endif
