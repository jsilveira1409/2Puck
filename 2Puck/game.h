/*
 * @file 		game.h
 * @brief		Main FSM controlling the game logic.
 * @author		Karl Khalil
 * @version		1.0
 * @date 		29 Apr 2022
 * @copyright	GNU Public License
 *
 */
#ifndef GAME_H_
#define GAME_H_


void game_init(void);
msg_t game_send_score(float score);


#endif /* GAME_H_ */
