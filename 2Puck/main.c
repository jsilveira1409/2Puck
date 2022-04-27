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

#include <pathing.h>
#include <photo.h>

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
    halInit();
    chSysInit();
    mpu_init();

    serial_start();
    //dac_start();
    usb_start();


    mic_start(&processAudioDataCmplx);
    //init_music();
    //init_pathing();
//    init_photo();
    while (1) {
    	float msg = 0;
//    	ReceiveInt16FromComputer(&msg,1);
    	chThdSleepMilliseconds(100);


    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
