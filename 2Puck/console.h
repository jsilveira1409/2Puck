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
msg_t console_send(msg_t msg);


#endif /* CONSOLE_H_ */
