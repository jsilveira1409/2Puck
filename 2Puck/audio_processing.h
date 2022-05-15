#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

void wait_note_played(void);
void wait_finish_playing(void);
void processAudioDataCmplx(int16_t *data, uint16_t num_samples);

#endif /* AUDIO_PROCESSING_H */


