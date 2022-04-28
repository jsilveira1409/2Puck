#ifndef MUSIC_H
#define MUSIC_H

enum jukebox{come_as_you_are, miss_you, killing_in_the_name_of, sold_the_world};

/*
 * Public Functions
 */
void init_music(void);
void play_song(uint8_t index);
void set_recording(uint8_t *data);


#endif
