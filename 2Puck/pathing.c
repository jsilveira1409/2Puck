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

#include <pathing.h>
#include <sensors/proximity.h>
#include <motors.h>
#include <arm_math.h>
#include "audio_processing.h"
#include "music.h"

#define WHEEL_DIST 				58		//mm
#define NSTEP_ONE_TURN			1000	//Nb steps for 1 turn
#define WHEEL_PERIMETER			130		//in mm
#define NB_OF_PHASES			4
#define RAD2DEG					(360/3.14159)
#define ANGLE_EPSILON			0.1
#define MIN_DISTANCE_2_TARGET	10
#define MIN_SPEED				300
#define MIN_IR_VAL				130
#define MIN_STEPS				3
#define ANGLE_RESOLUTION 		0.0000001
#define DISPLACEMENT_RESOLUTION 0.0000001
#define MAX_MOTOR_DISPLACEMENT	10
#define MIN_WALL_DIST			MIN_IR_VAL + 10

#define PLAYER1_X				(100)		//ptn de parenthese merci pour la nuit blanche
#define PLAYER1_Y				(500)
#define PLAYER2_X				(-150)
#define PLAYER2_Y				(300)
#define CENTER_X				10
#define CENTER_Y				10

/*
 * Thread pointer, so we can terminate it later
 */
static thread_t *ThdPtrPathing = NULL;
static BSEMAPHORE_DECL(sem_finished_pathing, TRUE);

enum {X_AXIS, Y_AXIS};
typedef enum {MOVING, ARRIVED} motor_state;

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
 * Vector between puck and target point
 */
static float dist[2] = {0,0};

/*
 * PID instances for the PID cmsis lib
 * steps_pid is used for the move functions, to soften the
 * steps advancement
 * angle_pid is used for the pathing functions, to improve
 *  the angle closing between dist and orientation
 */
static arm_pid_instance_f32 steps_pid;
static arm_pid_instance_f32 angle_pid;
static arm_pid_instance_f32 wall_pid;

static pathing_option current_option = WAIT;
/*
 * Static Functions
 */


static void pathing_finished(void){
	chBSemSignal(&sem_finished_pathing);
}

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
	static float beta = 0;  //angle entre l'axe y et le forward vector du ePuck
	float displacement = 0;	//Center of mass displacement
	float sin, cos;

	displacement = (left_pos + right_pos)/(float)2;
	beta += (left_pos - right_pos) / ((float)WHEEL_DIST);	//angle between the x axis and the forward pointing vector of the puck

	cos = arm_cos_f32(beta);
	sin = arm_sin_f32(beta);
	/*
	 * if both pos are equal, the angle should not be updated
	 * if both are opposite, it's turning on itself, so position shouldn't be updated
	 */
	if(left_pos != right_pos){
		update_orientation(cos, sin);
	}

	if(left_pos != (-right_pos)){
		position[X_AXIS] += displacement*sin;
		position[Y_AXIS] += displacement*cos;
	}
}

static float distance_to_target(float* cos_alpha, float* sin_alpha){

	float distance_mag = 0;
	float orientation_mag = 0;
	float mag = 0;
	/*
	 * temporary vector used for the cross product, which is not implemented
	 * by CMSIS's library, so we just invert and use the dot product
	 */
	float tmp_vector[2]={(orientation[Y_AXIS]), (-orientation[X_AXIS])};

	arm_sub_f32(target, position, dist, 2);
	arm_dot_prod_f32(orientation, dist, 2, cos_alpha);
	arm_dot_prod_f32(tmp_vector, dist, 2, sin_alpha);

	/*
	 * Orientation is unitary, so we divide by dist magnitude only
	 * TODO : check if it is actually the case
	 */
	arm_cmplx_mag_f32(dist, &distance_mag,1);
	arm_cmplx_mag_f32(orientation, &orientation_mag,1);
	mag = orientation_mag*distance_mag;
	*sin_alpha = (*sin_alpha)/mag;
	*cos_alpha = (*cos_alpha)/mag;
	return mag;
}

/*
 * Implements the PID for the motors, parameters in mm
 */
static void move (float left_pos, float right_pos){
	motor_state state = MOVING;
	/*
	 * Reset step counter
	 */
	right_motor_set_pos(0);
	left_motor_set_pos(0);

	register_path(left_pos, right_pos);

	float right_steps = right_pos * NSTEP_ONE_TURN / (WHEEL_PERIMETER);
	float left_steps = left_pos * NSTEP_ONE_TURN / (WHEEL_PERIMETER);

	volatile float output_right = 0;
	volatile float output_left = 0;

	float error_right = 0;
	float error_left  = 0;

	arm_pid_reset_f32(&steps_pid);

	while(state == MOVING){

		error_left  = left_steps -  (float)left_motor_get_pos();
		error_right = right_steps - (float)right_motor_get_pos();

		output_right = arm_pid_f32(&steps_pid, error_right);
		output_left = arm_pid_f32(&steps_pid, error_left);

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
		}else{
			right_motor_set_speed(0);
		}

		if(error_left >= MIN_STEPS){
			if(output_left < MIN_SPEED && output_left >= 0){
				left_motor_set_speed(MIN_SPEED);
			}else if(output_left > -MIN_SPEED && output_left <= 0){
				left_motor_set_speed(-MIN_SPEED);
			}else{
				left_motor_set_speed((int)output_left);
			}
		}else{
			left_motor_set_speed(0);
		}

		if(error_left < MIN_STEPS && error_right < MIN_STEPS){
			state = ARRIVED;
			right_motor_set_speed(0);
			left_motor_set_speed(0);
			right_motor_set_pos(0);
			left_motor_set_pos(0);
			return;
		}
	}
}

static ir_dir check_irs(float* ir_max_val){
	int ir[6] = {get_prox(5),get_prox(6),get_prox(7),
				 get_prox(0),get_prox(1),get_prox(2)};
	ir_dir max_ir_index = ir_hard_left;
	int max = ir[ir_hard_left];

	for(ir_dir i = ir_soft_left; i <= ir_hard_right; i++){
		if(ir[i] > max){
			max = ir[i];
			max_ir_index = i;
		}
	}

	if(max < MIN_IR_VAL){
		return none;
	}else{
		*ir_max_val = max;
		return max_ir_index;
	}
}


/*
 * We don't calculate the angle, as it implies calculating
 * acos and asin, which are not given by the dsp or the fpu. Therefore, we
 * try to close the angle by turning little by little until cos alpha is
 * more or less equal to one
 */

static void update_path(float cos_alpha, float sin_alpha){
	arm_pid_reset_f32(&wall_pid);
	volatile float move_l = 0, move_r = 0;
	volatile float error_sin = -sin_alpha;
	volatile float error_cos  = 1 - cos_alpha;

	error_sin = arm_pid_f32(&angle_pid, error_sin);
	error_cos = arm_pid_f32(&angle_pid, error_cos);

	move_l = (-error_sin + error_cos + 1)*MIN_STEPS;
	move_r = (error_sin  + error_cos + 1)*MIN_STEPS;
	if(move_l > MAX_MOTOR_DISPLACEMENT){
		move_l = MAX_MOTOR_DISPLACEMENT;
	}
	if(move_r > MAX_MOTOR_DISPLACEMENT){
		move_r = MAX_MOTOR_DISPLACEMENT;
	}
	move(move_l, move_r);
	return;
}

static void wall_follow(ir_dir ir,volatile float ir_val){
	volatile float error = MIN_WALL_DIST - ir_val;
	volatile float move_l = 0, move_r = 0;
	error = arm_pid_f32(&wall_pid, error);
	if(ir == ir_hard_left){
		move_l = MIN_STEPS - error;
		move_r = MIN_STEPS + error;
	}else if(ir == ir_hard_right){
		move_l = MIN_STEPS + error;
		move_r = MIN_STEPS - error;
	}
	move(move_l, move_r);

}



static void pathing(void){

	float distance = 0;
	float cos_alpha = 0, sin_alpha = 0;
	float ir_max_val = 0;
	ir_dir ir = 0;

	distance = distance_to_target(&cos_alpha, &sin_alpha);
	ir = check_irs(&ir_max_val);
	if(ir == ir_hard_left || ir == ir_hard_right){
		wall_follow(ir, ir_max_val);
	}
	if(distance < MIN_DISTANCE_2_TARGET){
		move(30,30);
		current_option = WAIT;
		pathing_finished();
		return;
	}else{
		ir = check_irs(&ir_max_val);
		switch(ir){
			case none:
				update_path(cos_alpha, sin_alpha);
				break;
			case ir_hard_left:
			case ir_hard_right:
				wall_follow(ir, ir_max_val);
				break;
			case ir_soft_left:
				move(5, 0);
				break;
			case ir_straight_left:
				move(5,-2);
				break;
			case ir_straight_right:
				move(-2,5);
				break;
			case ir_soft_right:
				move(0,5);
				break;

		}
	}
}


static void move_to_target(int16_t x_coord, int16_t y_coord){
	move(50,10);
	target[X_AXIS] = x_coord;
	target[Y_AXIS] = y_coord;
	current_option = PATHING;
}

static void dance(void){
	uint8_t note = 0;
//	wait_note_played();
	note = get_current_last_note();
	move(40,40);

//	switch(note){
//		case A1:
//			move(10,45);
//			break;
//		case AS1:
//			move(40,-20);
//			break;
//		case B1:
//			move(10,-20);
//			break;
//		case C1:
//			move(17,-60);
//			break;
//		case CS1:
//			move(15,15);
//			break;
//		case D1:
//			move(20,-20);
//			break;
//		case DS1:
//			move(-20,-20);
//			break;
//		case E1:
//			move(30,-1);
//			break;
//		case F1:
//			move(-7,2);
//			break;
//		case FS1:
//			move(8,-32);
//			break;
//		case G1:
//			move(9,5);
//			break;
//		case GS1:
//			move(2,12);
//			break;
//		default:
//			move(10,15);
//			break;
//	};
	return;
}

/*
 * THREADS
 */
static THD_WORKING_AREA(pathingWorkingArea, 256);

static THD_FUNCTION(ThdPathing, arg) {

	(void) arg;

	update_orientation(1,0);

	while (!chThdShouldTerminateX()) {
		switch (current_option){
			case WAIT:
				chThdSleepMilliseconds(1000);
				break;
			case DANCE:
				dance();
				break;
			case PATH_TO_PLAYER1:
				move_to_target(PLAYER1_X,PLAYER1_Y);
				break;
			case PATH_TO_PLAYER2:
				move_to_target(PLAYER2_X,PLAYER2_Y);
				break;
			case RECENTER:
				move_to_target(CENTER_X, CENTER_Y);
				break;
			case PATHING:
				pathing();
				break;
		};
	}
	chThdExit(0);
}



/*
 * Public Functions
 */
void pathing_init(){
	motors_init();

	proximity_start();
	calibrate_ir();

	steps_pid.Ki = 0;
	steps_pid.Kd = 0.1;
	steps_pid.Kp = 2;

	angle_pid.Ki = 1;
	angle_pid.Kd = 0.1;
	angle_pid.Kp = 5;

	wall_pid.Ki = 0;
	wall_pid.Kd = 0.001;
	wall_pid.Kp = 0.01;

	arm_pid_init_f32(&steps_pid, 0);
	arm_pid_init_f32(&angle_pid, 0);
	arm_pid_init_f32(&wall_pid, 0);

	ThdPtrPathing = chThdCreateStatic(pathingWorkingArea, sizeof(pathingWorkingArea),
	                           NORMALPRIO, ThdPathing, NULL);
}

void pathing_stop(){
	chThdTerminate(ThdPtrPathing);
}
void pathing_set(pathing_option option){
	current_option = option;
}

void pathing_wait_finish(void){
	chBSemWait(&sem_finished_pathing);
}
