#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <pathing.h>
#include <sensors/proximity.h>
#include <motors.h>
#include <arm_math.h>


enum { 	hard_left = 0, 	soft_left, straight_left,
		straight_right, soft_right, hard_right, none};

/*
 * VARIABLES ET CONSTANTES
 */
#define WHEEL_DIST 		52		//mm
#define NSTEP_ONE_TURN	1000	//Nb steps for 1 turn
#define WHEEL_PERIMETER	130		//in mm
#define NB_OF_PHASES	4
#define RAD2DEG			(360/3.14)

/*
 * Position values are in mm
 */

#define TARGET_X = 200
#define TARGET_Y = 200

/*
 * Current position of the puck, center of axis of the wheels
 */
volatile  static float pos_x = 0;
volatile static float pos_y = 0;
volatile static float angle = 0;

static uint8_t arrived = 0;
/*
 * PID instance for the PID cmsis
 */
static arm_pid_instance_f32 pid;


/*
 * THREADS
 */


static THD_WORKING_AREA(pathingWorkingArea, 256);

static THD_FUNCTION(pathing, arg) {


	while (true) {
		while(pos_x <= 100){

			int ir_max = 0;
			uint8_t ir_index = 0;
			ir_index = radar(&ir_max);

			switch(ir_index){
				case none:
					move(10,10);
					break;
				case hard_left:
					move(10,0);
					break;
				case soft_left:
					move(5,0);
					break;
				case straight_left:
					move(10,-10);
					break;
				case straight_right:
					move(-10,10);
					break;
				case soft_right:
					move(0,5);
					break;
				case hard_right:
					move(0,10);
					break;
				default:
					move(0,0);
					break;
			}
		}
	}
}


/*
 * FUNCTIONS
 */


uint8_t radar(int* ir_max){

	int ir[6] = {get_prox(5),get_prox(6),get_prox(7),
				 get_prox(0),get_prox(1),get_prox(2)};

	int max = ir[hard_left];
	uint8_t index_max = 0;

	for(uint8_t i = 1; i < hard_right; i++){
		if(ir[i] > max){
			max = ir[i];
			index_max = i;
		}
	}
	*ir_max = max;

	if(max < 250){
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
	right_motor_set_pos(0);
	left_motor_set_pos(0);

	register_path(left_pos, right_pos);

	float right_steps = right_pos * NSTEP_ONE_TURN / (WHEEL_PERIMETER);
	float left_steps = left_pos * NSTEP_ONE_TURN / (WHEEL_PERIMETER);

	volatile float output_right = 0;
	volatile float output_left = 0;

	float error_right = 0;
	float error_left  = 0;

	arm_pid_reset_f32(&pid);

	while(state == 0){

		error_left  = left_steps -  (float)left_motor_get_pos();
		error_right = right_steps - (float)right_motor_get_pos();

		output_right = arm_pid_f32(&pid, error_right);
		output_left = arm_pid_f32(&pid, error_left);

		arm_abs_f32(&error_right,&error_right,1);
		arm_abs_f32(&error_left,&error_left,1);

		if( error_right >= 2){
			if(output_right < 250 && output_right > 0){
				right_motor_set_speed(250);
			}else if (output_right > -250 && output_right < 0){
				right_motor_set_speed(-250);
			}else{
				right_motor_set_speed((int)output_right);
			}
		}

		if(error_left >= 2){
			if(output_left < 250 && output_left > 0){
				left_motor_set_speed(250);
			}else if(output_left < -250 && output_left < 0){
				left_motor_set_speed(-250);
			}else{
				left_motor_set_speed((int)output_left);
			}
		}

		if(error_left < 2 && error_right < 2){
			state = 1;
			right_motor_set_speed(0);
			left_motor_set_speed(0);
			right_motor_set_pos(0);
			left_motor_set_pos(0);
			return;
		}
	}
}

/*
 * Register Path
 */

void register_path( float left_pos,  float right_pos){
	volatile float displacement = (float)(left_pos + right_pos)/2;						//Center of mass displacement
	volatile float sin, cos;

	angle += (right_pos - left_pos) / ((float)WHEEL_DIST);	//angle between the x axis and the forward pointing vector of the puck

	/*
	 * Due to floating point intrinsic arithmetics, it generally never gives a zero
	 */
	if(angle < 0.001){
		angle = 0;
	}
	if(displacement < 0.001){
		displacement = 0;
	}
	cos = arm_cos_f32(angle);
	sin = arm_sin_f32(angle);

	pos_x += displacement*cos;
	pos_y += displacement*sin;

	chprintf((BaseSequentialStream *)&SD3, " %f \r \n", angle);
}


void init_pathing(){
	motors_init();

	proximity_start();
	calibrate_ir();

	pid.Kd = 0.5;
	pid.Ki = 0;
	pid.Kp = 2;

	arm_pid_init_f32(&pid, 0);

	(void) chThdCreateStatic(pathingWorkingArea, sizeof(pathingWorkingArea),
	                           NORMALPRIO, pathing, NULL);
}



