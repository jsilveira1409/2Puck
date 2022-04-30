#ifndef MUSIC_H
#define MUSIC_H


typedef enum {
	COME_AS_YOU_ARE,
	MISS_YOU,
	KILLING_IN_THE_NAME_OF,
	SOLD_THE_WORLD
}song_selection;

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
