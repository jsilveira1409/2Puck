#ifndef		PATHING_H
#define 	PATHING_H

void init_pathing(void);
void move( float right_pos, float left_pos);
uint8_t radar(int* ir_max);
void register_path(volatile float left_pos,volatile  float right_pos);
float distance_to_target(float* cos_alpha);
void move_to_target(float cos_alpha, float distance);
void update_orientation(float cos, float sin);
#endif
