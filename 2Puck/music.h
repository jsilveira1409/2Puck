#ifndef MUSIC_H
#define MUSIC_H

enum jukebox{come_as_you_are, miss_you, killing_in_the_name_of, sold_the_world};

void init_music(void );
int16_t check_note_sequence(uint8_t song_index);
int16_t check_note_order(uint8_t song_index);
void set_recording(uint8_t *data);

#endif
