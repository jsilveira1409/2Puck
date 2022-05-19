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

static uint8_t red = 0;
static uint8_t green = 0;
static uint8_t blue = 0;

static bool led_state[NUM_RGB_LED];

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

static bool random_state(void){
	rng_init();
	uint32_t random = rng_get();
	bool random_state = (random % 2);
	rng_stop();
	return random_state;
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
		for(rgb_led_name_t i=0; i<NUM_RGB_LED; i++){
			led_state[i] = random_state();
		}
	}
	chThdExit(MSG_OK);
}

static THD_WORKING_AREA(waLedUpdateThd, 128);
static THD_FUNCTION(ledUpdateThd, arg) {

	(void) arg;

	bool state = 0;

	while(!chThdShouldTerminateX()){
		if(state==0){
			for(rgb_led_name_t led=0; led<NUM_RGB_LED; led++){
				(led_state[led]==1) ? set_rgb_led(led, red, green, blue) : set_rgb_led(led, 0, 0, 0);
			}
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
		for(led_name_t led = 0; led <NUM_LED; led++){
			set_led(led, 0);
			chThdSleepMilliseconds(300);
			set_led(led, 1);
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
	lightshow_circle_init();
}

/*
 * @brief Stops the RGB lightshow.
 *
 */
void lightshow_stop(void){
	chThdTerminate(ptrLightshowThd);
	chThdTerminate(ptrLedUpdateThd);
	lightshow_circle_stop();
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
