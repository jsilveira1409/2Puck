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
#define MAX_MOTOR_DISPLACEMENT	10
#define MIN_WALL_DIST			MIN_IR_VAL + 10

#define PLAYER1_X				(100)
#define PLAYER1_Y				(500)
#define PLAYER2_X				(-150)
#define PLAYER2_Y				(300)
#define CENTER_X				0
#define CENTER_Y				0

static thread_t *ThdPtrPathing = NULL;
static BSEMAPHORE_DECL(sem_finished_pathing, TRUE);

enum {X_AXIS, Y_AXIS};
typedef enum {MOVING, ARRIVED} motor_state_t;

typedef enum {
		IR_HARD_LEFT = 0,
		IR_SOFT_LEFT,
		IR_STRAIGHT_LEFT,
		IR_STRAIGHT_RIGHT,
		IR_SOFT_RIGHT,
		IR_HARD_RIGHT,
		NONE
}ir_dir_t;

static arm_pid_instance_f32 steps_pid;
static arm_pid_instance_f32 angle_pid;
static arm_pid_instance_f32 wall_pid;

static float target[2] = {0,0};			// Target position for the end of the pathing
static float position[2] = {0,0};    	// Current position of the puck, center of axis of the wheels
static float orientation[2] = {0,1}; 	// Orientation of the forward vector of the puck (same direction as the TOF)


/*
 * Static Functions
 */

/*
 * @brief registers path by calculating beta and the ePucks displacement
 * as a function of left_pos and right_pos. It then updates the position
 * and orientation arrays.
 * @param [in] float left_pos : distance to advance the left wheel, in mm.
 * @param [in] float right_pos : distance to advance the left wheel, in mm.
 */

static void register_path(float left_pos,  float right_pos){
	static float beta = 0;  //angle entre l'axe y et le forward vector du ePuck
	float displacement = 0;	//Center of mass displacement
	float sin_beta, cos_beta;

	displacement = (left_pos + right_pos)/(float)2;
	beta += (left_pos - right_pos) / ((float)WHEEL_DIST);

	cos_beta = arm_cos_f32(beta);
	sin_beta = arm_sin_f32(beta);

	if(left_pos != right_pos){
		orientation[X_AXIS] = sin_beta;
		orientation[Y_AXIS] = cos_beta;
	}

	if(left_pos != (-right_pos)){
		position[X_AXIS] += displacement*sin_beta;
		position[Y_AXIS] += displacement*cos_beta;
	}
}

/*
 *@brief calculates the distances from the center of the
 *ePuck to the target by substracting the target array to the
 *position array, and calculating the result's magnitude
 *@param [in/out] float* dist : result of the substraction of
 *arrays
 *@return float
 */

static float distance_to_target(float* dist){
	float distance_mag = 0;

	arm_sub_f32(target, position, dist, 2);
	arm_cmplx_mag_f32(dist, &distance_mag,1);

	return distance_mag;
}

/*
 * @brief implements the PID for the motors, controlling
 * their speed depending on the error between the target number
 * of steps and the current one, parameters in mm.
 * @param [in] float left_pos : distance to advance the left wheel, in mm.
 * @param [in] float right_pos : distance to advance the right wheel, in mm.
 */
static void move(float left_pos, float right_pos){
	motor_state_t state = MOVING;
	/** Reset step counter **/
	right_motor_set_pos(0);
	left_motor_set_pos(0);

	register_path(left_pos, right_pos);

	float right_steps = right_pos * NSTEP_ONE_TURN / WHEEL_PERIMETER;
	float left_steps = left_pos * NSTEP_ONE_TURN / WHEEL_PERIMETER;

	float output_right = 0, error_right = 0;
	float output_left = 0, 	error_left = 0;

	arm_pid_reset_f32(&steps_pid);

	while(state == MOVING){

		error_left  = left_steps -  (float)left_motor_get_pos();
		error_right = right_steps - (float)right_motor_get_pos();

		output_right = arm_pid_f32(&steps_pid, error_right);
		output_left = arm_pid_f32(&steps_pid, error_left);

		arm_abs_f32(&error_right,&error_right,1);
		arm_abs_f32(&error_left,&error_left,1);

		if(error_right >= MIN_STEPS){
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
/*
 * @brief verifies which IR sensor returns the highest value,
 * with which we can deduce the obstacle's direction. If this value
 * is smaller than MIN_IR_VAL, the robot ignores it
 * @param [in/out] float* ir_max_val : highest value between the IR
 * sensors
 * @return ir_dir_t
 */

static ir_dir_t check_ir_dir(float* ir_max_val){
	int ir[6] = {get_prox(5),get_prox(6),get_prox(7),
				 get_prox(0),get_prox(1),get_prox(2)};
	ir_dir_t max_ir_index = IR_HARD_LEFT;
	int max = ir[IR_HARD_LEFT];

	for(ir_dir_t i = IR_SOFT_LEFT; i <= IR_HARD_RIGHT; i++){
		if(ir[i] > max){
			max = ir[i];
			max_ir_index = i;
		}
	}

	if(max < MIN_IR_VAL){
		return NONE;
	}else{
		*ir_max_val = max;
		return max_ir_index;
	}
}


/*
 * @brief updates the pathing variables of the robot. Calculates the cos_alpha
 * and sin_alpha, which then are used to determine to which direction it should
 * turn. We don't calculate the angle, as it implies calculating acos and asin,
 * which are not given by the dsp or the fpu.
 * @param [in/out] float* dist : array containing the distance vector, from the
 * robot to the target.
 * @param [in] float distance_mag : magnitude of dist vector
 *
 */
static void update_path(float* dist, float distance_mag){
	float move_l = 0, move_r = 0;
	float cos_alpha = 0, sin_alpha = 0;
	float orientation_mag = 0, mag = 0;

	/* temporary vector to do the vector product with a scalar product, by inverting and adding a sign */

	float tmp_vector[2] = {(orientation[Y_AXIS]), (-orientation[X_AXIS])};

	arm_pid_reset_f32(&wall_pid);
	arm_dot_prod_f32(orientation, dist, 2, &cos_alpha);
	arm_dot_prod_f32(tmp_vector, dist, 2, &sin_alpha);
	arm_cmplx_mag_f32(orientation, &orientation_mag, 1);

	mag = orientation_mag * distance_mag;
	sin_alpha = sin_alpha / mag;
	cos_alpha = cos_alpha / mag;

	float error_sin = 0 - sin_alpha;
	float error_cos = 1 - cos_alpha;

	error_sin = arm_pid_f32(&angle_pid, error_sin);
	error_cos = arm_pid_f32(&angle_pid, error_cos);

	move_l = (-error_sin + error_cos + 1) * MIN_STEPS;
	move_r = (error_sin  + error_cos + 1) * MIN_STEPS;

	if(move_l > MAX_MOTOR_DISPLACEMENT){
		move_l = MAX_MOTOR_DISPLACEMENT;
	}

	if(move_r > MAX_MOTOR_DISPLACEMENT){
		move_r = MAX_MOTOR_DISPLACEMENT;
	}

	move(move_l, move_r);
	return;
}

/*
 * @brief wall following function, implements a PID that keeps the robot at a certain
 * distance from the wall, using the left or the right IR sensor
 * @param [in] ir_dir_t ir : direction of the IR sensor closest to the wall, either
 * IR_HARD_LEFT or IR_HARD_RIGHT
 * @param [in] float ir_val : value of the IR sensor, which is used to compute the
 * PID's error
 *
 */
static void wall_follow(ir_dir_t ir, float ir_val){
	//TODO: implement conversion between ir value and distance
	float error = MIN_WALL_DIST - ir_val;
	float move_l = 0, move_r = 0;
	error = arm_pid_f32(&wall_pid, error);
	if(ir == IR_HARD_LEFT){
		move_l = MIN_STEPS - error;
		move_r = MIN_STEPS + error;
	}else if(ir == IR_HARD_RIGHT){
		move_l = MIN_STEPS + error;
		move_r = MIN_STEPS - error;
	}
	move(move_l, move_r);
}

/*
 * @brief pathing function that guides the robot from where it currently
 * is to the target. It checks the IR sensors and the distance to the target
 * to, respectively, either avoid the obstacle or stop s(whenever the distance
 * is smaller than MIN_DISTANCE_2_TARGET).
 * @return pathing_option_t
 */
static pathing_option_t pathing(void){
	float dist[2] = {0,0};					// Vector between puck and target point
	float distance_mag = 0;
	float ir_max_val = 0;

	ir_dir_t ir = NONE;
	pathing_option_t option = PATHING;

	distance_mag = distance_to_target(dist);
	ir = check_ir_dir(&ir_max_val);

	if(distance_mag < MIN_DISTANCE_2_TARGET){
		move(30,30);
		option = PATHING_FINISHED;
	}else{
		switch(ir){
			case NONE:
				update_path(dist, distance_mag);
				break;
			case IR_HARD_LEFT:
			case IR_HARD_RIGHT:
				wall_follow(ir, ir_max_val);
				break;
			case IR_SOFT_LEFT:
				move(5, 0);
				break;
			case IR_STRAIGHT_LEFT:
				move(5,-2);
				break;
			case IR_STRAIGHT_RIGHT:
				move(-2,5);
				break;
			case IR_SOFT_RIGHT:
				move(0,5);
				break;
		}
	}
	return option;
}

/*
 * @brief sets the target array values to the ones passed to
 * the function.
 * @param[in] int16_t x_coord : x coordinate of the target
 * @param[in] int16_t y_coord : y coordinate of the target
 * @return pathing_option_t
 */
static pathing_option_t set_target(int16_t x_coord, int16_t y_coord){
	target[X_AXIS] = x_coord;
	target[Y_AXIS] = y_coord;
	return PATHING;
}

/*
 * THREADS
 */
static THD_WORKING_AREA(pathingWorkingArea, 256);

static THD_FUNCTION(ThdPathing, arg) {

	(void)arg;

	pathing_option_t current_option = (pathing_option_t)arg;

	while (!chThdShouldTerminateX()) {
		switch (current_option){
			case PATHING_WAIT:
				chThdSleepMilliseconds(2000);
				//TODO:should wait for a msg from game here
				break;
			case PATHING_FINISHED:
				chBSemSignal(&sem_finished_pathing);
				pathing_stop();
				break;
			case PATH_TO_PLAYER1:
				current_option = set_target(PLAYER1_X,PLAYER1_Y);
				break;
			case PATH_TO_PLAYER2:
				current_option = set_target(PLAYER2_X,PLAYER2_Y);
				break;
			case PATH_RECENTER:
				current_option = set_target(CENTER_X, CENTER_Y);
				break;
			case PATHING:
				current_option = pathing();
				break;
		};
	}
	chThdExit(MSG_OK);
}

/*
 * Public Functions
 */

/*
 * @brief Initializes the pathing thread and all the required
 * subsystems. ALso initialiyes the three PIDs and their values
 * @param[in] pathing_option_t option :
 */
void pathing_init(pathing_option_t option){
	motors_init();
	chThdSleepMilliseconds(100);
	proximity_start();
	chThdSleepMilliseconds(100);
	calibrate_ir();
	chThdSleepMilliseconds(100);

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
	                           NORMALPRIO, ThdPathing, (void*)option);
}

/*
 * Public Functions
 */

/*
 * @brief terminates the pathing thread
 */
void pathing_stop(){
	chThdTerminate(ThdPtrPathing);
}

/*
 * @brief
 */
void pathing_wait_finish(void){
	chBSemWait(&sem_finished_pathing);
}
