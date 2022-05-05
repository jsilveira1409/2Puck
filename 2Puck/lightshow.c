#include <leds.h>
#include <music.h>
#include <audio_processing.h>

/*
 * Static Functions
 */
static void toggle(uint8_t * state){
	if(*state == 0) *state = 1;
		else *state = 0;
}

static void circle(uint32_t time_to_sleep){
	static uint8_t led_state = 0;
	for(led_name_t led = LED1; led <NUM_LED; led++){
		set_led(led,led_state);
		chThdSleepMilliseconds(time_to_sleep);
	}
	toggle(&led_state);
}

static void blink_body(uint32_t time_to_sleep){
	static uint8_t led_state = 0;
	set_body_led(led_state);
	chThdSleepMilliseconds(time_to_sleep);
	toggle(&led_state);
}


/*
 * Threads
 */
static THD_WORKING_AREA(lightshowWA, 128);
static THD_FUNCTION(lightshow_thd, arg) {
	clear_leds();
	(void) arg;
	while(1){

	}
}

static THD_WORKING_AREA(redledsWA, 64);
static THD_FUNCTION(redleds_thd, arg) {
	clear_leds();
	(void) arg;
	while(1){
		circle(500);
	}
}

static THD_WORKING_AREA(bodyledWA, 64);
static THD_FUNCTION(bodyled_thd, arg) {
	clear_leds();
	(void) arg;
	blink_body(50);
	while(1){
		blink_body(50);
		blink_body(50);
		wait_note_played();
	}
}


/*
 * Public Functions
 */

void lightshow_init(void){
//	chThdCreateStatic(lightshowWA, sizeof(lightshowWA), NORMALPRIO, lightshow_thd, NULL);
//	chThdCreateStatic(redledsWA, sizeof(redledsWA), NORMALPRIO, redleds_thd, NULL);
	chThdCreateStatic(bodyledWA, sizeof(bodyledWA), NORMALPRIO, bodyled_thd, NULL);
}

void lighshow_stop(void){

}

