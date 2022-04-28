#ifndef MUSIC_H
#define MUSIC_H

enum jukebox{come_as_you_are, miss_you, killing_in_the_name_of, sold_the_world};

/*
 * Public Functions
 */
void init_music(void);
void play_song(uint8_t index);
void set_recording(uint8_t *data);
uint8_t random_song(void);
void wait_finish_music(void);
int16_t get_score(void);
/*
 * Static functions
 */
int16_t check_note_order(uint8_t index);
int16_t check_note_sequence(uint8_t index);
#endif
