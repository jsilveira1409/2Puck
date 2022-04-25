#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>
#include <inttypes.h>


#include <photo.h>


#define X_start					50
#define	Y_start					0
#define PHOTO_WIDTH				500
#define PHOTO_HEIGHT			2
#define	BYTES_PER_PIXEL			2
#define IMAGE_BUFFER_SIZE		(PHOTO_WIDTH*PHOTO_HEIGHT)	//Size in uint16
#define	MAX_LINES_2_SEND		500

//semaphore
static BSEMAPHORE_DECL(line_ready_sem, TRUE);

//Threads
static THD_WORKING_AREA(waTakePhoto, 256);
static THD_FUNCTION(TakePhoto, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;
    static uint16_t line_cnt = 1;
    static uint8_t send = 1;


	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);


    while(1){
    	uint8_t *img_buff_ptr;

    	po8030_advanced_config(FORMAT_RGB565, X_start, (Y_start + line_cnt), PHOTO_WIDTH, PHOTO_HEIGHT, SUBSAMPLING_X1, SUBSAMPLING_X1);
    	dcmi_prepare();

        //starts a capture

		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		dcmi_capture_stop();

		if(line_cnt <= MAX_LINES_2_SEND){
			/*
			 * the library gets 2 lines at once
			 */
			line_cnt += 2;
		}else{
			/*
			 * Finished sending the picture
			 */
			send = 0;
		}
		if(send == 1){
			img_buff_ptr = dcmi_get_last_image_ptr();
			/*
			 * As each pixel has 2 bytes of color, total width becomes 2* PHOTO_WIDTH for the data we need to send
			 */
			SendUint8ToComputer(img_buff_ptr, (2*PHOTO_WIDTH));
			SendUint8ToComputer((img_buff_ptr+ 2*PHOTO_WIDTH), (2*PHOTO_WIDTH));
		}

		//signals an image has been captured
		chBSemSignal(&line_ready_sem);
    }
}

void init_photo(void){
	dcmi_start();
	po8030_start();
	chThdCreateStatic(waTakePhoto, sizeof(waTakePhoto), NORMALPRIO, TakePhoto, NULL);
}

void SendUint8ToComputer(uint8_t* data, uint16_t size)
{
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, size);
}


