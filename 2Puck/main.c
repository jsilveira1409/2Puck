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
#include <audio/audio_thread.h>
#include <music.h>
#include <spi_comm.h>
#include <audio_processing.h>
#include <fft.h>
#include <communications.h>
#include <arm_math.h>
#include <audio/audio_thread.h>
#include <audio/play_sound_file.h>
#include <sdio.h>
#include <game.h>
#include <fat.h>
#include <leds.h>

#include <lightshow.h>

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
    dcmi_start();
    po8030_start();

	sdio_start();
	dac_start();
	playSoundFileStart();


	/*
	 * SD card init does not like being inside music_init
	 * it segfaults
	 */
	while(!mountSDCard()){
		set_body_led(1);
		chThdSleepMilliseconds(200);
		set_body_led(0);
		chThdSleepMilliseconds(200);
	}

//	lightshow_init();
    game_init();

	while (1) {

	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
