#ifndef MUSIC_H
#define MUSIC_H

void music_init(void);
void music_stop(void);
void play_song(void);
void stop_song(void);
void music_listen(void);
msg_t music_send_freq(float freq);
bool music_is_playing(void);

#endif
