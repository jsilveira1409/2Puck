#include "ch.h"
#include "hal.h"
#include <main.h>

#include <communications.h>
#include <music.h>
#include <leds.h>

/*
 * Messages received and sent to and from the PC
 */
enum{
	/*
	 * Received messages
	 */
	none,
	start_game,
	player1_play,
	player2_play,
	/*
	 * Send message
	 */
	chosen_song,
	player1_finish,
	player2_finish,
	send_winner,
	send_photo,
	finished
}message;


static THD_WORKING_AREA(communicationWorkingArea, 128);

static THD_FUNCTION(communication, arg) {


	uint8_t msg = none;
	const uint8_t score1 = 11;
	const uint8_t score2 = 12;

	while (msg != player2_finish) {
		if(msg == none || msg == chosen_song || msg  == player1_finish){
			ReceiveModeFromComputer((BaseSequentialStream *) &SD3, &msg);
			set_led(LED1, 1);
		}else if(msg == start_game){
			uint8_t chosen_song = come_as_you_are;
			SendUint8ToComputer(&chosen_song, 1);
			msg = chosen_song;
			set_led(LED3, 1);
		}else if(msg == player1_play){
			/*
			 * Start player 1 recording, compute score and send
			 */
			SendUint8ToComputer(&score1, 1);
			mic_start(&processAudioDataCmplx);
			init_music();
			msg = player1_finish;
			set_led(LED5,1);
		}else if (msg == player2_play){
			/*
			 * Start player 2 recording, compute score and send
			 */
			SendUint8ToComputer(&score2, 1);
			msg = player2_finish;
			set_led(LED7,1);
		}
		chThdSleepMilliseconds(200);
	}
	set_body_led(1);

}





/*
*       Sends floats numbers to the computer
*/
void SendFloatToComputer(BaseSequentialStream* out, float* data, uint16_t size) 
{	
	chSequentialStreamWrite(out, (uint8_t*)"START", 5);
	chSequentialStreamWrite(out, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite(out, (uint8_t*)data, sizeof(float) * size);
}
/*
*       Sends floats numbers to the computer
*/
void SendUint8ToComputer(uint8_t* data, uint16_t size)
{
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, sizeof(uint8_t)*size);
}
/*
*	Receives int16 values from the computer and fill a float array with complex values.
*	Puts 0 to the imaginary part. Size is the number of complex numbers.
*	=> data should be of size (2 * size)
*/
uint16_t ReceiveInt16FromComputer(BaseSequentialStream* in, float* data, uint16_t size){

	volatile uint8_t c1, c2;
	volatile uint16_t temp_size = 0;
	uint16_t i=0;

	uint8_t state = 0;
	while(state != 5){

        c1 = chSequentialStreamGet(in);

        //State machine to detect the string EOF\0S in order synchronize
        //with the frame received
        switch(state){
        	case 0:
        		if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        	case 1:
        		if(c1 == 'T')
        			state = 2;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        	case 2:
        		if(c1 == 'A')
        			state = 3;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        	case 3:
        		if(c1 == 'R')
        			state = 4;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        	case 4:
        		if(c1 == 'T')
        			state = 5;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        }
        
	}

	c1 = chSequentialStreamGet(in);
	c2 = chSequentialStreamGet(in);

	// The first 2 bytes is the length of the datas
	// -> number of int16_t data
	temp_size = (int16_t)((c1 | c2<<8));

	if((temp_size/2) == size){
		for(i = 0 ; i < (temp_size/2) ; i++){

			c1 = chSequentialStreamGet(in);
			c2 = chSequentialStreamGet(in);

			data[i*2] = (int16_t)((c1 | c2<<8));	//real
			data[(i*2)+1] = 0;										//imaginary
		}
	}

	return temp_size/2;

}


uint16_t ReceiveModeFromComputer(BaseSequentialStream* in, uint8_t* data){

	volatile uint8_t c1;
	volatile uint16_t temp_size = 0;
	uint16_t i=0;

	uint8_t state = 0;
	while(state != 5){

        c1 = chSequentialStreamGet(in);
        switch(state){
        	case 0:
        		if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        		break;
        	case 1:
        		if(c1 == 'T')
        			state = 2;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        		break;
        	case 2:
        		if(c1 == 'A')
        			state = 3;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        		break;
        	case 3:
        		if(c1 == 'R')
        			state = 4;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        		break;
        	case 4:
        		if(c1 == 'T')
        			state = 5;
        		else if(c1 == 'S')
        			state = 1;
        		else
        			state = 0;
        		break;
        }

	}
	c1 = chSequentialStreamGet(in);
	*data = (uint8_t)c1;
	return temp_size;
}


void init_communication(){
	(void) chThdCreateStatic(communicationWorkingArea, sizeof(communicationWorkingArea), NORMALPRIO, communication, NULL);
}

