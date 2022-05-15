/*
 * pathing.h
 *
 *  Created on: 12 Apr 2022
 *      Author: Joaquim Silveira
 */


#ifndef		PATHING_H_
#define 	PATHING_H_


typedef enum{
	PATH_TO_PLAYER1,
	PATH_TO_PLAYER2,
	PATHING,
	PATHING_FINISHED
}pathing_option_t;

void pathing_init(pathing_option_t option);
void pathing_stop(void);
void pathing_set(pathing_option_t option);
void pathing_wait_finish(void);

#endif	/* PATHING_H_ */

