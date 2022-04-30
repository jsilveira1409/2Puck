#ifndef		PATHING_H
#define 	PATHING_H

void init_pathing(void);
void move( float right_pos, float left_pos);
uint8_t check_irs(int* ir_max);
void register_path(volatile float left_pos,volatile  float right_pos);
float distance_to_target(float* cos_alpha);
void move_to_target(float cos_alpha);
void update_orientation(float cos, float sin);
void pathing(uint8_t *state);
void update_target(int16_t x_coord, int16_t y_coord);
void recenter_puck(void);
#endif
