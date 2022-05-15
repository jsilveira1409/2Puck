/*
 * @file 		lightshow.c
 * @brief		Led's pattern control library.
 * @author		Karl Khalil
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		18 Apr 2022
 * @copyright	GNU Public License
 *
 */

#include <leds.h>
#include <spi_comm.h>
#include "lightshow.h"
#include "rng.h"
#include "music.h"

static thread_t* ptrLightshowThd = NULL;
static thread_t* ptrLedUpdateThd = NULL;
static thread_t* ptrCircleThd 	 = NULL;

uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

/*
 * Static Functions
 */

/*
 * @brief chooses a random number between 0 and 255.
 *
 * @return chosen random number
 */
static uint8_t random_color(void){
	rng_init();
	uint32_t random = rng_get();
	uint8_t random_color = (random % 256);
	rng_stop();
	return random_color;
}


/*
 * Threads
 */
static THD_WORKING_AREA(waLightshowThd, 128);
static THD_FUNCTION(lightshowThd, arg) {

	(void) arg;

	while(!chThdShouldTerminateX()){
		music_wait_new_note();
		red = random_color();
		green = random_color();
		blue = random_color();
	}
	chThdExit(MSG_OK);
}

static THD_WORKING_AREA(waLedUpdateThd, 128);
static THD_FUNCTION(ledUpdateThd, arg) {

	(void) arg;

	bool state = 0;

	while(!chThdShouldTerminateX()){
		if(state==0){
			set_rgb_led(LED2, red, green, blue);
			set_rgb_led(LED4, red, green, blue);
			set_rgb_led(LED6, red, green, blue);
			set_rgb_led(LED8, red, green, blue);
			set_body_led(1);
			state = 1;
		}else{
			set_rgb_led(LED2, 0, 0, 0);
			set_rgb_led(LED4, 0, 0, 0);
			set_rgb_led(LED6, 0, 0, 0);
			set_rgb_led(LED8, 0, 0, 0);
			set_body_led(0);
			state = 0;
		}
		chThdSleepMilliseconds(400);
	}

	set_rgb_led(LED2, 0, 0, 0);
	set_rgb_led(LED4, 0, 0, 0);
	set_rgb_led(LED6, 0, 0, 0);
	set_rgb_led(LED8, 0, 0, 0);
	set_body_led(0);
	state = 0;
	chThdExit(MSG_OK);
}

static THD_WORKING_AREA(waCircleThd, 128);
static THD_FUNCTION(circleThd, arg) {

	(void) arg;

	set_led(LED1,1);
	set_led(LED3,1);
	set_led(LED5,1);
	set_led(LED7,1);

	while(!chThdShouldTerminateX()){
		for(led_name_t led = LED1; led <NUM_LED; led++){
			set_led(led, 0);
			set_led(led-1, 1);
			chThdSleepMilliseconds(300);
		}
	}
	chThdExit(MSG_OK);
}

/*
 * Public Functions
 */

/*
 * @brief Changes RGB lights in a random pattern for each note played.
 *
 */
void lightshow_init(void){
	spi_comm_start();
	ptrLightshowThd = chThdCreateStatic(waLightshowThd, sizeof(waLightshowThd),
										NORMALPRIO, lightshowThd, NULL);
	ptrLedUpdateThd = chThdCreateStatic(waLedUpdateThd, sizeof(waLedUpdateThd),
											NORMALPRIO, ledUpdateThd, NULL);
}

/*
 * @brief Stops the RGB lightshow.
 *
 */
void lightshow_stop(void){
	chThdTerminate(ptrLightshowThd);
	chThdTerminate(ptrLedUpdateThd);
}

/*
 * @brief Play the red LED in a turning circle.
 *
 */
void lightshow_circle_init(void){
	ptrCircleThd = chThdCreateStatic(waCircleThd, sizeof(waCircleThd),
											NORMALPRIO, circleThd, NULL);

}

/*
 * @brief Stops the red LED lightshow.
 *
 */
void lightshow_circle_stop(void){
	chThdTerminate(ptrCircleThd);
}
