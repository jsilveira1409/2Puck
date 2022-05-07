
#include <hal.h>
#include <main.h>
#include <communications.h>


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

	 uint8_t c1, c2;
	 uint16_t temp_size = 0;
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

	 uint8_t c1;
	 uint16_t temp_size = 0;

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
