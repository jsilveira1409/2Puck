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



/*
 * For the position and target vectors
 */
enum {X_AXIS, Y_AXIS};

/*
 * For the IR obstacle position relative to the puck
 */
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
#define ANGLE_EPSILON	0.05
#define MIN_DISTANCE_2_TARGET	5
#define MIN_SPEED		200


#define TARGET_X	(140)		//ptn de parenthese merci pour la nuit blanche
#define TARGET_Y	(300)



/*
 * Current position of the puck, center of axis of the wheels
 */
static float position[2] = {0,0};
/*
 * Target position for the end of the pathing
 */
static float target[2] = {TARGET_X,TARGET_Y};
static float orientation[2] = {0,0};

/*
 * Angle between puck's orientation and vector between itself
 * and the target point
 */
static float alpha = 0;



/*
 * PID instance for the PID cmsis
 */
static arm_pid_instance_f32 pid;


/*
 * THREADS
 */


static THD_WORKING_AREA(pathingWorkingArea, 256);

static THD_FUNCTION(pathing, arg) {
	uint8_t arrived = 0;
	move(5,5);
	while (true) {

		while(arrived == 0){
			int ir_max = 0;
			float distance = 0;
			float cos_alpha = 0;
			uint8_t ir_index = 0;
			distance = distance_to_target(&cos_alpha);
			chprintf((BaseSequentialStream *)&SD3, " %f %f  \r \n", cos_alpha, distance);

			if(distance < MIN_DISTANCE_2_TARGET){
				arrived = 1;
				move(10,10);
				chprintf((BaseSequentialStream *)&SD3, " Arrived \r \n");
			}else{
				ir_index = radar(&ir_max);
				switch(ir_index){
					case none:
						/*
						 * No obstacles in front, so move_to_target should
						 * be called here, move is called inside move_to_target
						 * in this case
						 */
						move_to_target(cos_alpha, distance);
						break;
					case hard_left:
						move(4, 2);
						break;
					case soft_left:
						move(4, 0);
						break;
					case straight_left:
						move(5,-5);
						break;
					case straight_right:
						move(-5,5);
						break;
					case soft_right:
						move(0,4);
						break;
					case hard_right:
						move(2,4);
						break;
					default:
						move(5,5);
						break;
				}
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

	if(max < 130){
		return none;
	}else{
		return index_max;
	}
}


/*
 * Implements the PID for the motors, parameters in mm
 */
void move (float left_pos, float right_pos){
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

		if( error_right >= 5){
			if(output_right < MIN_SPEED && output_right >= 0){
				right_motor_set_speed(MIN_SPEED);
			}else if (output_right > -MIN_SPEED && output_right <= 0){
				right_motor_set_speed(-MIN_SPEED);
			}else{
				right_motor_set_speed((int)output_right);
			}
		}

		if(error_left >= 5){
			if(output_left < MIN_SPEED && output_left >= 0){
				left_motor_set_speed(MIN_SPEED);
			}else if(output_left > -MIN_SPEED && output_left <= 0){
				left_motor_set_speed(-MIN_SPEED);
			}else{
				left_motor_set_speed((int)output_left);
			}
		}

		if(error_left < 5 && error_right < 5){
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
	volatile float displacement = (left_pos + right_pos)/(float)2;						//Center of mass displacement
	volatile float sin, cos;

	alpha += (left_pos - right_pos) / ((float)WHEEL_DIST);	//angle between the x axis and the forward pointing vector of the puck

	/*
	 * Due to floating point intrinsic arithmetics, it generally never gives a zero
	 */

	if((alpha >= 0 && alpha < 0.1) || (alpha <= 0 && alpha > -0.1)){
		alpha = 0;
	}
	if((displacement < 1 && displacement >= 0) || (displacement > -1 && displacement <= 0)){
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

	//chprintf((BaseSequentialStream *)&SD3, " %f \r \n", alpha);
}

/*
 * Basically a polar to cartesian transform, we consider it to be unitary
 * as we don't care for the magnitude (for now). The orientation is of course
 * a 2D vector centered in puck and pointing towards the camera/TOF direction
 */
void update_orientation(float cos, float sin){
	orientation[X_AXIS] = sin;
	orientation[Y_AXIS] = cos;
	return;
}


/*
 * move_to_target. we don't calculate the angle, as it implies calculating
 * acos and asin, which are not given by the dsp or the fpu. Therefore, we
 * try to close the angle by turning little by little until cos alpha is
 * more or less equal to one
 */

void move_to_target(float cos_alpha, float distance){
	float move_l = 0, move_r = 0;

	if( (cos_alpha < (1-ANGLE_EPSILON) && (cos_alpha > 0))){
		if(position[X_AXIS] >= 0){
			move_l = 5;
			move_r = -5;
		}else{
			move_l = -5;
			move_r = 5;
		}
	}else if ((cos_alpha >= (1 - ANGLE_EPSILON))){
		move_l = 5;
		move_r = 5;
	}
	/*
	 * move() is called in the end because the move positions for each
	 * wheel are calculated before
	 */
	move(move_l, move_r);
	return;
}

float distance_to_target(float* cos_alpha){
	float dist[2] = {0,0};					//Vector between puck and target point
	float mag = 0;

	arm_sub_f32(target, position, dist, 2);
	arm_dot_prod_f32(orientation, dist, 2, cos_alpha);
	/*
	 * Orientation is unitary, so we divide by dist magnitude only
	 */
	arm_cmplx_mag_f32(dist, &mag,1);
	*cos_alpha = (*cos_alpha)/mag;
	return mag;
}

void init_pathing(void){
	motors_init();

	proximity_start();
	calibrate_ir();

	pid.Kd = 0.0001;
	pid.Ki = 0;
	pid.Kp = 1;

	arm_pid_init_f32(&pid, 0);

	(void) chThdCreateStatic(pathingWorkingArea, sizeof(pathingWorkingArea),
	                           NORMALPRIO, pathing, NULL);
}


