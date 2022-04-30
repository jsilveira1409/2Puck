/*
 * 				  P: target(TARGET_X, TARGET_Y)
 * 			   	 ^
 * 				/
 * 		+y	^  /dist: vector between epuck current pos and target
 * 			| /
 * 			|/
 * 	origin 	o ----> +x
 * 	  =
 * 	center of
 * 	puck
 *
 * 	ALL POSITION VALUES ARE IN mm
 */


#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <pathing.h>
#include <sensors/proximity.h>
#include <motors.h>
#include <arm_math.h>
#include <leds.h>

#define WHEEL_DIST 				52		//mm
#define NSTEP_ONE_TURN			1000	//Nb steps for 1 turn
#define WHEEL_PERIMETER			130		//in mm
#define NB_OF_PHASES			4
#define RAD2DEG					(360/3.14)
#define ANGLE_EPSILON			0.01
#define MIN_DISTANCE_2_TARGET	20
#define MIN_SPEED				200
#define MIN_IR_VAL				125
#define MIN_STEPS				2
#define ANGLE_RESOLUTION 		0.000001
#define DISPLACEMENT_RESOLUTION 0.000001

#define PLAYER1_X				(200)		//ptn de parenthese merci pour la nuit blanche
#define PLAYER1_Y				(500)
#define PLAYER2_X				(-200)
#define PLAYER2_Y				(300)
#define CENTER_X				0
#define CENTER_Y				0

enum {X_AXIS, Y_AXIS};

//typedef enum{
//	moving,
//	dancing,
//	waiting,
//	finished
//}state;

/*
 * For the IR obstacle position relative to the puck
 */
typedef enum {
		ir_hard_left = 0,
		ir_soft_left,
		ir_straight_left,
		ir_straight_right,
		ir_soft_right,
		ir_hard_right,
		none
}ir_dir;

/*
 * Current position of the puck, center of axis of the wheels
 */
static float position[2] = {0,0};
/*
 * Target position for the end of the pathing
 */
static float target[2] = {0,0};
/*
 * Orientation of the forward vector of the puck (same direction
 * as the TOF)
 */
static float orientation[2] = {0,0};
/*
 * PID instance for the PID cmsis
 */
static arm_pid_instance_f32 pid;

static pathing_option current_option = WAIT;
/*
 * Static Functions
 */

/*
 * Basically a polar to cartesian transform, we consider it to be unitary
 * as we don't care for the magnitude (for now). The orientation is of course
 * a 2D vector centered in puck and pointing towards the camera/TOF direction
 */
static void update_orientation(float cos, float sin){
	orientation[X_AXIS] = sin;
	orientation[Y_AXIS] = cos;
	return;
}


static void register_path( float left_pos,  float right_pos){
	static float alpha = 0;
	float displacement = (left_pos + right_pos)/(float)2;						//Center of mass displacement
	float sin, cos;

	alpha += (left_pos - right_pos) / ((float)WHEEL_DIST);	//angle between the x axis and the forward pointing vector of the puck

	/*
	 * Due to floating point intrinsic arithmetics, it generally never gives a zero
	 */

	if((displacement < DISPLACEMENT_RESOLUTION && displacement >= 0) || (displacement > -DISPLACEMENT_RESOLUTION && displacement < 0)){
		displacement = 0;
	}
	cos = arm_cos_f32(alpha);
	sin = arm_sin_f32(alpha);

	/*
	 * if both pos are equal, the angle should not be updated
	 */
	if(left_pos != right_pos){
		update_orientation(cos, sin);
	}

	position[X_AXIS] += displacement*sin;
	position[Y_AXIS] += displacement*cos;
}

static float distance_to_target(float* cos_alpha){
	float dist[2] = {0,0};					//Vector between puck and target point
	float mag = 0;

	arm_sub_f32(target, position, dist, 2);
	arm_dot_prod_f32(orientation, dist, 2, cos_alpha);
	/*
	 * Orientation is unitary, so we divide by dist magnitude only
	 * TODO : check if it is actually the case
	 */
	arm_cmplx_mag_f32(dist, &mag,1);
	*cos_alpha = (*cos_alpha)/mag;
	return mag;
}

/*
 * Implements the PID for the motors, parameters in mm
 */
static void move (float left_pos, float right_pos){
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

		if( error_right >= MIN_STEPS){
			if(output_right < MIN_SPEED && output_right >= 0){
				right_motor_set_speed(MIN_SPEED);
			}else if (output_right > -MIN_SPEED && output_right <= 0){
				right_motor_set_speed(-MIN_SPEED);
			}else{
				right_motor_set_speed((int)output_right);
			}
		}

		if(error_left >= MIN_STEPS){
			if(output_left < MIN_SPEED && output_left >= 0){
				left_motor_set_speed(MIN_SPEED);
			}else if(output_left > -MIN_SPEED && output_left <= 0){
				left_motor_set_speed(-MIN_SPEED);
			}else{
				left_motor_set_speed((int)output_left);
			}
		}

		if(error_left < MIN_STEPS && error_right < MIN_STEPS){
			state = 1;
			right_motor_set_speed(0);
			left_motor_set_speed(0);
			right_motor_set_pos(0);
			left_motor_set_pos(0);
			return;
		}
	}
}

static ir_dir check_irs(int* ir_max){
	int ir[6] = {get_prox(5),get_prox(6),get_prox(7),
				 get_prox(0),get_prox(1),get_prox(2)};

	int max = ir[ir_hard_left];
	ir_dir index_max = 0;

	for(ir_dir i = ir_soft_left; i < ir_hard_right; i++){
		if(ir[i] > max){
			max = ir[i];
			index_max = i;
		}
	}
	*ir_max = max;
	if(max < MIN_IR_VAL){
		return none;
	}else{
		return index_max;
	}
}


/*
 * We don't calculate the angle, as it implies calculating
 * acos and asin, which are not given by the dsp or the fpu. Therefore, we
 * try to close the angle by turning little by little until cos alpha is
 * more or less equal to one
 */

static void move_to_target(float cos_alpha){
	float move_l = 0, move_r = 0;

	if( (cos_alpha < (1 - ANGLE_EPSILON) && (cos_alpha >= 0)) || (cos_alpha < (ANGLE_EPSILON - 1) && (cos_alpha < 0))  ){
		if((position[X_AXIS] - target[X_AXIS]) >= 0){
			move_l = -5;	//TURN LEFT
			move_r = 5;
		}else{
			move_l = 5;		//TURN RIGHT
			move_r = -5;
		}
	}else if (cos_alpha >= (1 - ANGLE_EPSILON) && (cos_alpha > 0)){
		move_l = 5;
		move_r = 5;
	}else if(cos_alpha >= (ANGLE_EPSILON - 1) && (cos_alpha < 0) ){
		if((position[X_AXIS] - target[X_AXIS]) >= 0){
			move_l = -5;
			move_r = 5;
		}else{
			move_l = 5;
			move_r = -5;
		}
	}
	move(move_l, move_r);
	return;
}


static void pathing(uint8_t *state){
	int ir_max = 0;
	float distance = 0;
	float cos_alpha = 0;
	ir_dir ir = 0;

	distance = distance_to_target(&cos_alpha);

	if(distance < MIN_DISTANCE_2_TARGET){
		move(20,20);
		*state = waiting;
		return;
	}else{
		ir = check_irs(&ir_max);
		switch(ir){
			case none:
				set_led(LED1, 1);
				move_to_target(cos_alpha);
				set_led(LED1, 0);
				break;
			case ir_hard_left:
				set_led(LED1, 1);
				move_to_target(cos_alpha);
				set_led(LED1, 0);
				break;
			case ir_soft_left:
				move(5, 1);
				break;
			case ir_straight_left:
				move(4,-4);
				break;
			case ir_straight_right:
				move(-4,4);
				break;
			case ir_soft_right:
				move(1,5);
				break;
			case ir_hard_right:
				set_led(LED1, 1);
				move_to_target(cos_alpha);
				set_led(LED1, 0);
				break;
		}
	}
}


static void update_target(int16_t x_coord, int16_t y_coord){
	target[X_AXIS] = x_coord;
	target[Y_AXIS] = y_coord;
}


/*
 * THREADS
 */
static THD_WORKING_AREA(pathingWorkingArea, 256);

static THD_FUNCTION(ThdPathing, arg) {

	(void) arg;

	update_orientation(1,0);

	while (true) {

	}
}



/*
 * Public Functions
 */
void pathing_init(void){
	motors_init();

	proximity_start();
	calibrate_ir();

	pid.Ki = 0;
	pid.Kd = 0.00001;
	pid.Kp = 1.2;

	arm_pid_init_f32(&pid, 0);

	(void) chThdCreateStatic(pathingWorkingArea, sizeof(pathingWorkingArea),
	                           NORMALPRIO, ThdPathing, NULL);
}

void pathing_set(pathing_option option){
	current_option = option;
}
