/*
 * @file 		audio_processing.c
 * @brief		Microphone sample processing library.
 * @author		Karl Khalil
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		12 Apr 2022
 * @copyright	GNU Public License
 *
 */

#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H


void wait_note_played(void);
void wait_finish_playing(void);
void processAudioDataCmplx(int16_t *data, uint16_t num_samples);


#endif /* AUDIO_PROCESSING_H */


