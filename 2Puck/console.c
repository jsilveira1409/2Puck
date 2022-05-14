/*
 * console.c
 *
 *  Created on: 7 May 2022
 *      Author: Karl Khalil
 *      		Joaquim Silveira
 */

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <stdio.h>
#include "console.h"

static thread_t* console;

/*
 * @brief	Console server thread,
 * @param[in] arg	Stream where message must be printed.
 *
 */
static THD_WORKING_AREA(waConsoleServerThread, 1024);

static THD_FUNCTION(ConsoleServerThread, arg) {
  BaseSequentialStream *stream = (BaseSequentialStream *)arg;

  while (!chThdShouldTerminateX()) {
    /* Waiting for a queued message then retrieving it.*/
    thread_t* tp = chMsgWait();
    const char* msg = (const char*)chMsgGet(tp);

    /* Printing message the message prefixing it with a time stamp.*/
    chprintf(stream, "%010d: %s\r\n", chVTGetSystemTime(), msg);

    /* Sending back an acknowledge.*/
    chMsgRelease(tp, MSG_OK);
  }
  chThdExit(MSG_OK);
}


/**
 * @brief   Initialize UART3 communication and Console Server.
 */
void console_init(void) {

	static SerialConfig ser_cfg = {
		115200,
		0,
		0,
		0,
	};

	sdStart(&SD3, &ser_cfg); // UART3.

  /* Starting the console thread making it print over the serial
     port.*/
  console = chThdCreateStatic(waConsoleServerThread, sizeof(waConsoleServerThread),
                              NORMALPRIO + 1, ConsoleServerThread, (void *)&SD3);

  (void)chMsgSend(console, (msg_t)"Communication started.");
}

/**
 * @brief   Stops UART3 communication and Console Server.
 *
 * @return  Console Thread exit message
 *
 */
msg_t console_stop(void){
	sdStop(&SD3);
	chThdTerminate(console);
	return chThdWait(console);
}

/**
 * @brief  Wrapper that uses ChibiOS messages to send a string to serial.
 *
 * @param[in] msg	Message to send to console.
 * @return 			Console Server return message.
 */
msg_t console_send_string(const char* msg){
	return chMsgSend(console, (msg_t)msg);
}

msg_t console_send_int(int num, char* msg){
	char buffer[20];
	sprintf(buffer,"%s %d",msg,num);
	return chMsgSend(console, (msg_t)buffer);
}

char console_get_char(char* input_msg){
	console_send_string(input_msg);
	return chSequentialStreamGet(&SD3);

}
