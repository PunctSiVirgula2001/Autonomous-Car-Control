#ifndef SERVOCONTROL
#define SERVOCONTROL

#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_task_wdt.h"


/*Settings for LEDC */
#define LEDC_SERVO_TIMER          	LEDC_TIMER_0
#define LEDC_SERVO_GPIO       	  	(27)
#define LEDC_SERVO_CH0_CHANNEL    	LEDC_CHANNEL_0
#define LEDC_SERVO_DUTY_RESOLUTION  LEDC_TIMER_10_BIT // Resolution of 10 bits

#define LEDC_MOTOR_TIMER 			LEDC_TIMER_1
#define LEDC_MOTOR_CH1_CHANNEL 		LEDC_CHANNEL_1
#define LEDC_MOTOR_GPIO 			(4)

#define LEDC_Motor_And_Servo_MODE  LEDC_HIGH_SPEED_MODE
#define LEDC_Motor_And_Servo_FREQ  50   // 50Hz

/* Servo functions */
void init_servo_pwm();
void update_servo_pwm(int pulse_width_us);
void changeSTEER(int value);

/* Motor functions */
void init_motor_pwm();
void update_motor_pwm(unsigned int pulse_width_us);
void changeMotorSpeed(int value);

#endif
