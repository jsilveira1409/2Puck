#include <ch.h>
#include <hal.h>
#include <memory_protection.h>
#include <audio/play_sound_file.h>
#include <sdio.h>
#include <fat.h>
#include <leds.h>
#include "main.h"
#include "game.h"
#include "lightshow.h"
#include "music.h"
#include "pathing.h"


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

//	sdio_start();
//	playSoundFileStart();
//
//
//	/*
//	 * SD card init does not like being inside music_init
//	 * it segfaults
//	 */
//	while(!mountSDCard()){
//		set_body_led(1);
//		chThdSleepMilliseconds(200);
//		set_body_led(0);
//		chThdSleepMilliseconds(200);
//	}
//
////	lightshow_init();
    game_init();

	while (1) {
		;
		//TODO: EXIT() ?
	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
