/*
 * DcMotor.h
 *
 *  Created on: 18 iun. 2024
 *      Author: racov
 */

#ifndef INCLUDE_MOTORS_DCMOTOR_H_
#define INCLUDE_MOTORS_DCMOTOR_H_

#include "motors_common.h"


/*Pwm settings for dc motor */
#define LEDC_MOTOR_TIMER 			LEDC_TIMER_1
#define LEDC_MOTOR_CH1_CHANNEL 		LEDC_CHANNEL_1
#define LEDC_MOTOR_DUTY_RESOLUTION 	LEDC_TIMER_16_BIT

#define DC_MOTOR_GPIO 				(4)

#define MAX_MOTOR_FW_DUTY_US 		(1650)
#define MIN_MOTOR_FW_DUTY_US 		(1535)
#define MAX_MOTOR_BW_DUTY_US 		(1430)
#define MIN_MOTOR_BW_DUTY_US 		(1200)

/* Motor functions */
#define QUEUE_SIZE_SPEED 		    20
#define QUEUE_SIZE_DATATYPE_SPEED (sizeof(int))

void init_motor_pwm();
void carControl_calibrate_motor();
void update_motor_pwm(unsigned int pulse_width_us);
void changeMotorSpeed(int value);



#endif /* INCLUDE_MOTORS_DCMOTOR_H_ */
