#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <pathing.h>
#include <sensors/proximity.h>
#include <motors.h>
#include <arm_math.h>

enum { 	hard_left, 	soft_left, straight_left,
		straight_right, soft_right, hard_right, none};

/*
 * VARIABLES ET CONSTANTES
 */
#define NSTEP_ONE_TURN	1000	//Nb steps for 1 turn
#define WHEEL_PERIMETER	130		//in mm
#define NB_OF_PHASES	4

/*
 * Position values are in mm
 */

#define TARGET_X = 200
#define TARGET_Y = 200

/*
 * Current position of the puck
 */
static int16_t pos_x = 0;
static int16_t pos_y = 0;
/*
 * PID instance for the PID cmsis
 */
static arm_pid_instance_f32 pid;


/*
 * THREADS
 */


static THD_WORKING_AREA(pathingWorkingArea, 128);

static THD_FUNCTION(pathing, arg) {

	static uint8_t arrived = 0;

	while (true) {

		if(arrived == 0){
			int ir_max = 0;
			uint8_t ir_index = 0;
			ir_index = radar(&ir_max);
			switch(ir_index){
				case none:
					left_motor_set_speed(200);
					right_motor_set_speed(200);
					break;
				case hard_left:
					left_motor_set_speed(50);
					right_motor_set_speed(-100);
					break;
				case soft_left:
					left_motor_set_speed(50);
					right_motor_set_speed(-200);
					break;
				case straight_left:
					left_motor_set_speed(200);
					right_motor_set_speed(-200);
					break;
				case straight_right:
					left_motor_set_speed(-200);
					right_motor_set_speed(200);
					break;
				case soft_right:
					left_motor_set_speed(-200);
					right_motor_set_speed(50);
					break;
				case hard_right:
					left_motor_set_speed(-100);
					right_motor_set_speed(50);
					break;
				default:
					left_motor_set_speed(0);
					right_motor_set_speed(0);
					break;

			}



//			while(ir8 < 100 || ir1 < 100){
//				ir8 = get_prox(7);
//				ir1 = get_prox(0);
//				left_motor_set_speed(200);
//				right_motor_set_speed(200);
////				move(5, 5);
//				chThdSleepMilliseconds(10);
//			}

//			arrived = 1;
		}
		chThdSleepMilliseconds(10);
	}
}


/*
 * FUNCTIONS
 */


uint8_t radar(int* ir_max){
	int ir[6] = {get_prox(6),get_prox(7),get_prox(8),
				 get_prox(1),get_prox(2),get_prox(3)};
	int max = ir[hard_left];
	uint8_t index_max = 0;

	for(uint8_t i = 1; i < hard_right; i++){
		if(ir[i] > max){
			max = ir[i];
			index_max = i;
		}
	}
	if(max < 140){
		return none;
	}else{
		return index_max;
	}
}


/*
 * Implements the PID for the motors, parameters in mm
 */
void move( float left_pos, float right_pos){
	uint8_t state = 0;
	float right_steps = right_pos * NSTEP_ONE_TURN / (WHEEL_PERIMETER);
	float left_steps = left_pos * NSTEP_ONE_TURN / (WHEEL_PERIMETER);

	float output_right = 0;
	float output_left = 0;

	float error_right = 0;
	float error_left  = 0;

	arm_pid_reset_f32(&pid);

	while(state == 0){
		error_right = right_steps - (float)right_motor_get_pos();
		error_left  = left_steps -  (float)left_motor_get_pos();
		output_right = arm_pid_f32(&pid, error_right);
		output_left = arm_pid_f32(&pid, error_left);

		if(abs(error_right) >= 2){
			if(abs(output_right) < 200){
				right_motor_set_speed(200);
			}else{
				right_motor_set_speed((int)output_right);
			}
		}
		if(abs(error_left) >= 2){
			if(abs(output_left) < 200){
				left_motor_set_speed(200);
			}else{
				left_motor_set_speed((int)output_left);
			}
		}
		if(abs(error_left) < 5 && abs(error_right) < 5){
			state = 1;
			right_motor_set_speed(0);
			left_motor_set_speed(0);
			right_motor_set_pos(0);
			left_motor_set_pos(0);
		}
	}
}


void init_pathing(){
	motors_init();

	proximity_start();
	calibrate_ir();

	pid.Kd = 0.3;
	pid.Ki = 0;
	pid.Kp = 1;

	arm_pid_init_f32(&pid, 0);

	(void) chThdCreateStatic(pathingWorkingArea, sizeof(pathingWorkingArea),
	                           NORMALPRIO, pathing, NULL);
}



