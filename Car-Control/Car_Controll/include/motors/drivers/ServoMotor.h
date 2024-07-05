/*
 * ServoMotor.h
 *
 *  Created on: 18 iun. 2024
 *      Author: racov
 */

#ifndef INCLUDE_MOTORS_SERVOMOTOR_H_
#define INCLUDE_MOTORS_SERVOMOTOR_H_

#include "motors_common.h"


#define LEDC_SERVO_GPIO       	  	(27)
/*Pwm settings for servo LEDC */
#define LEDC_SERVO_TIMER          	LEDC_TIMER_0
#define LEDC_SERVO_CH0_CHANNEL    	LEDC_CHANNEL_0
#define LEDC_SERVO_DUTY_RESOLUTION  LEDC_TIMER_16_BIT

/* Pwm settings for servo general*/
#define MAX_SERVO_DUTY_US 			(1800)
#define MIN_SERVO_DUTY_US 			(1200)

/* Servo functions */
void init_servo_pwm();
void update_servo_pwm(int pulse_width_us);
void changeSTEER(int value);



#endif /* INCLUDE_MOTORS_SERVOMOTOR_H_ */
