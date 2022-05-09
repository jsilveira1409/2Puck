#ifndef MUSIC_H
#define MUSIC_H

typedef enum {
	COME_AS_YOU_ARE,
	MISS_YOU,
	SOLD_THE_WORLD,
	SEVEN_NATION,
	NEXT_EPISODE
}song_selection_t;

typedef enum {
	A, AS, B, C, CS, D, DS,E, F, FS, G, GS, CHROMATIC_SIZE
}chromatic_scale;


/*
 * Public Functions
 */
song_selection_t music_init(void);
void music_stop(void);
void play_song(uint8_t index);
void stop_song(void);
void set_recording(uint8_t *data);
void wait_finish_music(void);
int16_t get_score(void);
song_selection_t choose_random_song(void);

#endif
