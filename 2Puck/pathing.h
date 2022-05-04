#ifndef		PATHING_H
#define 	PATHING_H

typedef enum{
	PATH_TO_PLAYER1,
	PATH_TO_PLAYER2,
	DANCE,
	RECENTER,
	WAIT,
	PATHING,
}pathing_option;


void pathing_init(void);
void pathing_stop(void);
void pathing_set(pathing_option option);
void pathing_wait_finish(void);

#endif
