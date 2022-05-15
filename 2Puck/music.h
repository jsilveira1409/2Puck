/*
 * @file 		music.h
 * @brief		Musical processing of the frequency data.
 * @author		Karl Khalil
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		20 Apr 2022
 * @copyright	GNU Public License
 *
 */

#ifndef MUSIC_H
#define MUSIC_H

void music_init(void);
void music_stop(void);
void play_song(void);
void stop_song(void);
void music_listen(void);
msg_t music_send_freq(float freq);
bool music_is_playing(void);
void music_wait_new_note(void);

#endif /*MUSIC_H*/
