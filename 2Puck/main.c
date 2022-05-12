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
#include "console.h"

int main(void)
{
    halInit();
    chSysInit();
    mpu_init();

	sdio_start();
	playSoundFileStart();


	console_init();

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

