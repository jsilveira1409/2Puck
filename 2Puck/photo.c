#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>
#include <inttypes.h>
#include <leds.h>
#include <photo.h>

#include "communications.h"

#include <photo.h>


#define X_start					50
#define	Y_start					0
<<<<<<< HEAD
#define PHOTO_WIDTH				340
#define PHOTO_HEIGHT			4
=======
#define PHOTO_WIDTH				450
#define PHOTO_HEIGHT			2
>>>>>>> dev
#define	BYTES_PER_PIXEL			2
#define IMAGE_BUFFER_SIZE		(PHOTO_WIDTH*PHOTO_HEIGHT)	//Size in uint16
#define	MAX_LINES_2_SEND		200

//semaphore
static BSEMAPHORE_DECL(line_ready_sem, TRUE);
static BSEMAPHORE_DECL(photo_finished_sem, TRUE);

//Threads
static THD_WORKING_AREA(waTakePhoto, 512);
static THD_FUNCTION(ThdPhoto, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;
    static uint16_t line_cnt = 0;
    static uint8_t send = 1;


	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);


    while(!chThdShouldTerminateX()){
    	uint8_t *img_buff_ptr;
<<<<<<< HEAD

    	static uint8_t led = 0;
		set_led(LED3,led);
		if(led == 1) led = 0;
		else led = 1;

    	po8030_advanced_config(FORMAT_RGB565, X_start, (Y_start + line_cnt), PHOTO_WIDTH, PHOTO_HEIGHT,
    			SUBSAMPLING_X1, SUBSAMPLING_X1);
=======
    	po8030_advanced_config(FORMAT_RGB565, X_start, (Y_start + line_cnt),
    				PHOTO_WIDTH, PHOTO_HEIGHT, SUBSAMPLING_X1, SUBSAMPLING_X1);
>>>>>>> dev
    	dcmi_prepare();

    	//starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		dcmi_capture_stop();

		if(line_cnt <= MAX_LINES_2_SEND){
			/*
			 * the library gets 2 lines at once
			 *
			 */
			line_cnt += PHOTO_HEIGHT;
		}else{
			/*
			 * Finished sending the picture
			 */
			send = 0;
			chBSemSignal(&photo_finished_sem);
		}
		if(send == 1){
			img_buff_ptr = dcmi_get_last_image_ptr();
			/*
			 * As each pixel has 2 bytes of color, total width becomes 2* PHOTO_WIDTH for the data we need to send
			 */
<<<<<<< HEAD
			for(uint8_t i=0; i<PHOTO_HEIGHT;i++){
				SendUint8ToComputer((img_buff_ptr + (2*i*PHOTO_WIDTH)), (2*PHOTO_WIDTH));
//				SendUint8ToComputer((img_buff_ptr + 2*PHOTO_WIDTH), (2*PHOTO_WIDTH));
			}
		}else{
			chBSemSignal(&photo_finished_sem);
=======
			SendUint8ToComputer(img_buff_ptr, (2*PHOTO_WIDTH));
			SendUint8ToComputer((img_buff_ptr + 2*PHOTO_WIDTH), (2*PHOTO_WIDTH));
>>>>>>> dev
		}

		//signals an image has been captured
		chBSemSignal(&line_ready_sem);
    }
    chThdExit(0);
}

<<<<<<< HEAD
void photo_init(void){
	chThdCreateStatic(waTakePhoto, sizeof(waTakePhoto), NORMALPRIO, TakePhoto, NULL);
}

void photo_wait_finish(void){
=======
void init_photo(void){
	chThdCreateStatic(waTakePhoto, sizeof(waTakePhoto), NORMALPRIO + 2, ThdPhoto, NULL);
}

void stop_photo(void){
	chThdTerminate(&ThdPhoto);
	dcmi_capture_stop();
	free(dcmi_get_first_buffer_ptr);
	free(dcmi_get_second_buffer_ptr);

}

void wait_photo_finish(void){
>>>>>>> dev
	chBSemWait(&photo_finished_sem);
}
