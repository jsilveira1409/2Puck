/*
 * console.h
 *
 *  Created on: 7 May 2022
 *      Author: Karl Khalil
 *      		Joaquim Silveira
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

void console_init(void);
msg_t console_stop(void);
msg_t console_send_string(const char* msg);
msg_t console_send_int(int num, char* msg);
char console_get_char(char* input_msg);

#endif /* CONSOLE_H_ */
