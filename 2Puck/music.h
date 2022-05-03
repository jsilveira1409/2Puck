#ifndef MUSIC_H
#define MUSIC_H

typedef enum {
	COME_AS_YOU_ARE,
	MISS_YOU,
	KILLIN_IN_THE_NAME_OF,
	SOLD_THE_WORLD,
	SEVEN_NATION,
	NEXT_EPISODE
}song_selection;

typedef enum {
	A1, AS1, B1, C1, CS1, D1, DS1,E1, F1, FS1, G1, GS1,
	A2, AS2, B2, C2, CS2, D2, DS2,E2, F2, FS2, G2, GS2,
	A3, AS3, B3, C3, CS3, D3, DS3,E3, F3, FS3, G3, GS3,
	A4, AS4, B4, C4, CS4, D4, DS4,E4, F4, FS4, G4, GS4,
	A5, AS5, B5, C5, CS5, D5, DS5,E5, F5, FS5, G5, GS5,
}chromatic_scale;


/*
 * Public Functions
 */
void music_init(void);
void music_stop(void);
void play_song(uint8_t index);
void set_recording(uint8_t *data);
void wait_finish_music(void);
int16_t get_score(void);
song_selection get_song(void);
song_selection random_song(void );

#endif
