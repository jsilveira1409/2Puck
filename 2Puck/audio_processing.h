#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#define FFT_SIZE 		4096
#define NB_NOTES		60


static const uint16_t note_frequency[NB_NOTES] = {
/* 	  A   A#    B    C     C#    D     D#     E     F    F#     G   G#*/
	55,   58,  62,   65,   69,   73,   77,   82,   87,   92,   98,  104,
	110, 116, 124, 	131,  138, 	146,  155, 	165,  175, 	185,  196,  208,
	220, 233, 247,  262,  277,  294,  311,  330,  349,  370,  392,  415,
	440, 466, 494,  523,  554,  587,  622,  659,  698,  740,  784,  831,
	880, 932, 988, 1047, 1108, 1174, 1244, 1318, 1396, 1480, 1568, 1662
};


typedef enum {
	//2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
	LEFT_CMPLX_INPUT = 0,
	RIGHT_CMPLX_INPUT,
	FRONT_CMPLX_INPUT,
	BACK_CMPLX_INPUT,
	//Arrays containing the computed magnitude of the complex numbers
	LEFT_OUTPUT,
	RIGHT_OUTPUT,
	FRONT_OUTPUT,
	BACK_OUTPUT
} BUFFER_NAME_t;

/*
 * Public functions
 */
void processAudioDataCmplx(int16_t *data, uint16_t num_samples);
uint8_t* get_recording(void);
uint8_t get_current_last_note(void);
void wait_note_played(void);
void wait_finish_playing(void);
void find_note (int16_t index);

#endif /* AUDIO_PROCESSING_H */


