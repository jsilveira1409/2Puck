/*
 * @file 		console.h
 * @brief		Communication with serial monitor
 * @author		Karl Khalil
 * @author		Joaquim Silveira
 * @version		1.0
 * @date 		7 May 2022
 * @copyright	GNU Public License
 *
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

void console_init(void);
msg_t console_stop(void);
msg_t console_send_string(const char* msg);
msg_t console_send_int(int num, char* msg);
char console_get_char(char* input_msg);
void SendUint8ToComputer(uint8_t* data, uint16_t size);

#endif /* CONSOLE_H_ */
