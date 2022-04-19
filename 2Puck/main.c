#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "memory_protection.h"
#include <usbcfg.h>
#include <main.h>
#include <chprintf.h>
#include <motors.h>
#include <audio/microphone.h>
#include <music.h>

#include <audio_processing.h>
#include <fft.h>
#include <communications.h>
#include <arm_math.h>

static void serial_start(void)
{
	static SerialConfig ser_cfg = {
	    115200,
	    0,
	    0,
	    0,
	};

	sdStart(&SD3, &ser_cfg); // UART3.
}



int main(void)
{
	int16_t seq_score = 0;
	int16_t ord_score = 0;
    halInit();
    chSysInit();
    mpu_init();

    serial_start();

    usb_start();

    mic_start(&processAudioDataCmplx);


    while (1) {
    	seq_score = 0;
    	ord_score = 0;
    	wait_finish_playing();
    	set_recording(get_recording());
    	ord_score = check_note_order(0);
    	seq_score = check_note_sequence(0);
		chprintf((BaseSequentialStream *)&SD3, "sequence score :  %d \r \n", seq_score);
		//chprintf((BaseSequentialStream *)&SD3, "order score    :  %d \r \n", ord_score);
    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
