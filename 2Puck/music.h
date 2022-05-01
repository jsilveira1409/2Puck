#ifndef MUSIC_H
#define MUSIC_H

<<<<<<< Updated upstream
=======
enum jukebox{
	come_as_you_are,
	miss_you,
	killing_in_the_name_of,
	sold_the_world,
	seven_nation_army,
	next_episode};

/*
 * Enum for others functions to choose the song more explicitely,
 * Basically the song index in the songs array
 */
typedef enum {
	A1, AS1, B1, C1, CS1, D1, DS1,E1, F1, FS1, G1, GS1,
	A2, AS2, B2, C2, CS2, D2, DS2,E2, F2, FS2, G2, GS2,
	A3, AS3, B3, C3, CS3, D3, DS3,E3, F3, FS3, G3, GS3,
	A4, AS4, B4, C4, CS4, D4, DS4,E4, F4, FS4, G4, GS4,
	A5, AS5, B5, C5, CS5, D5, DS5,E5, F5, FS5, G5, GS5,
	X
}chromatic_scale;
>>>>>>> Stashed changes

/*
 * Public Functions
 */
void init_music(void);
void play_song(uint8_t index);
void set_recording(uint8_t *data);


#endif
