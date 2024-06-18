/*
 * motors_common.h
 *
 *  Created on: 18 iun. 2024
 *      Author: racov
 */

#ifndef INCLUDE_MOTORS_DRIVERS_MOTORS_COMMON_H_
#define INCLUDE_MOTORS_DRIVERS_MOTORS_COMMON_H_

#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_task_wdt.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_gen.h"
#include "driver/pulse_cnt.h"
#include "freertos/FreeRTOS.h"

#include <stdbool.h> // For bool type
#include <stdlib.h> // For atoi function
#include <string.h> // For strlen function

#define ON 0U
#define OFF 1U

#define MOTOR_MOCK_TEST OFF

#define MCPWM 0U
#define LEDC 1U

/*Set which pwm source to use: MCPWM or LEDC.*/
#define MOTOR_CONTROL MCPWM

/*Pwm settings that are shared between servo and dc motor */
#define LEDC_Motor_And_Servo_MODE  	LEDC_HIGH_SPEED_MODE
#define LEDC_Motor_And_Servo_FREQ  	50   // 50Hz




#endif /* INCLUDE_MOTORS_DRIVERS_MOTORS_COMMON_H_ */
