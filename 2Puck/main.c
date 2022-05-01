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

#include <pathing.h>
#include <photo.h>

#include <leds.h>
#include <game.h>
#include <fat.h>
#include <sdio.h>

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
//    dcmi_start();
//    po8030_start();
//    spi_comm_start();

//    init_photo();

//    init_music();
//    init_game();
//    pathing_init();
//    pathing_set(DANCE);

    sdio_start();
    dac_start();
    spi_comm_start();
    playSoundFileStart();


    while(!mountSDCard()){
    	set_body_led(1);
    	chThdSleepMilliseconds(200);
    	set_body_led(0);
		chThdSleepMilliseconds(200);
    }
    set_body_led(1);

    set_body_led(1);


    while (1) {
    	setSoundFileVolume(50);
    	playSoundFile("nextepisode.wav", SF_FORCE_CHANGE);
		waitSoundFileHasFinished();
    	set_body_led(1);
		chThdSleepMilliseconds(200);
		set_body_led(0);
		chThdSleepMilliseconds(200);
	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
