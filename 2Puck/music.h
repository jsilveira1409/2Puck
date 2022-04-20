#ifndef MUSIC_H
#define MUSIC_H



void init_music();
int16_t check_note_sequence(uint8_t song_index);
int16_t check_note_order(uint8_t song_index);
void set_recording(uint8_t *data);

#endif
