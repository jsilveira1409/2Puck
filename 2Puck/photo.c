#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>
#include <inttypes.h>


#include <photo.h>

#define PXTOCM 	1570

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

//Threads
static THD_WORKING_AREA(waTakePhoto, 256);
static THD_FUNCTION(TakePhoto, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 30, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

    while(1){
        //starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		//signals an image has been captured
		chBSemSignal(&image_ready_sem);

		//gets the pointer to the array filled with the last image in RGB565
//		img_buff_ptr = dcmi_get_last_image_ptr();

		//Send data to python script to reconstruct the image
		chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);


    }
}

void photo_start(void){
	chThdCreateStatic(waTakePhoto, sizeof(waTakePhoto), NORMALPRIO, TakePhoto, NULL);
}
