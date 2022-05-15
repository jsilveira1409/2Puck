/*
 * photo.c
 *
 *  Created on: 23 Apr 2022
 *      Author: Joaquim Silveira
 */

#include "ch.h"
#include "hal.h"
#include <main.h>
#include <camera/po8030.h>
#include <inttypes.h>
#include "photo.h"
#include "console.h"

static thread_t *ThdPtrPhoto = NULL;
#define X_start					50
#define	Y_start					0
#define PHOTO_WIDTH				250
#define PHOTO_HEIGHT			4
#define	MAX_LINES_2_SEND		350

//Semaphores
static BSEMAPHORE_DECL(line_ready_sem, TRUE);
static BSEMAPHORE_DECL(photo_finished_sem, TRUE);

//Thread
static THD_WORKING_AREA(photoWA, 512);

static THD_FUNCTION(photo_thd, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;
    static uint16_t line_cnt = 0;
    static uint8_t send = true;

    dcmi_start();
    po8030_start();

	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);

    while(!chThdShouldTerminateX()){
    	uint8_t *img_buff_ptr;

    	po8030_advanced_config(FORMAT_RGB565, X_start, (Y_start + line_cnt), PHOTO_WIDTH, PHOTO_HEIGHT,
    			SUBSAMPLING_X1, SUBSAMPLING_X1);

    	dcmi_prepare();

    	//starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		dcmi_capture_stop();

		if(line_cnt <= MAX_LINES_2_SEND){
			line_cnt += PHOTO_HEIGHT;
		}else{
			//Finished sending photo
			send = false;
			chBSemSignal(&photo_finished_sem);
		}
		if(send == true){
			img_buff_ptr = dcmi_get_last_image_ptr();
			/*  As each pixel has 2 bytes of color, total width becomes 2* PHOTO_WIDTH
			 *  for the data we need to send*/
			for(uint8_t i=0; i<PHOTO_HEIGHT;i++){
				SendUint8ToComputer((img_buff_ptr + (2*i*PHOTO_WIDTH)), (2*PHOTO_WIDTH));
			}
		}else{
			chBSemSignal(&photo_finished_sem);
		}

		//signals an image has been captured
		chBSemSignal(&line_ready_sem);
    }
    chThdExit(0);
}

/*
 * Public Functions
 */

/*
 * @brief 	Initializes the photo thread.
 */
void photo_init(void){
	ThdPtrPhoto = chThdCreateStatic(photoWA, sizeof(photoWA),
			NORMALPRIO+2, photo_thd, NULL);
}

/*
 * @brief	Stops the photo thread and frees the buffers used.
 */
void photo_stop(void){
	chThdTerminate(ThdPtrPhoto);
	dcmi_capture_stop();
	dcmi_free_buffers();
	//TODO: p08030_stop();
}

/*
 * @brief	Waits for the capture to be finished.
 */
void photo_wait_finish(void){
	chBSemWait(&photo_finished_sem);
}
