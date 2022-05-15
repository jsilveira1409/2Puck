#ifndef		PATHING_H
#define 	PATHING_H

typedef enum{
	PATH_TO_PLAYER1,
	PATH_TO_PLAYER2,
	PATH_RECENTER,
	PATHING,
	PATHING_FINISHED
}pathing_option_t;


void pathing_init(pathing_option_t option);
void pathing_stop(void);
void pathing_set(pathing_option_t option);
void pathing_wait_finish(void);

#endif
