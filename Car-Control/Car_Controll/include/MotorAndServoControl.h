#ifndef SERVOCONTROL
#define SERVOCONTROL

#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_task_wdt.h"
#include "driver/pulse_cnt.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_gen.h"

#include <stdbool.h> // For bool type
#include <stdlib.h> // For atoi function
#include <string.h> // For strlen function

#define MCPWM 0U
#define LEDC 1U

/*Set which pwm source to use: MCPWM or LEDC.*/
#define MOTOR_CONTROL MCPWM

/* ---------LEDC Settings-----------*/

/*Pwm settings for servo */
#define LEDC_SERVO_TIMER          	LEDC_TIMER_0
#define LEDC_SERVO_GPIO       	  	(27)
#define LEDC_SERVO_CH0_CHANNEL    	LEDC_CHANNEL_0
#define LEDC_SERVO_DUTY_RESOLUTION  LEDC_TIMER_16_BIT
#define MAX_SERVO_DUTY_US 			(1650)
#define MIN_SERVO_DUTY_US 			(1350)

/*Pwm settings for dc motor */
#define LEDC_MOTOR_TIMER 			LEDC_TIMER_1
#define LEDC_MOTOR_CH1_CHANNEL 		LEDC_CHANNEL_1
#define LEDC_MOTOR_GPIO 			(4)
#define LEDC_MOTOR_DUTY_RESOLUTION 	LEDC_TIMER_16_BIT
#define MAX_MOTOR_FW_DUTY_US 		(1650)
#define MIN_MOTOR_FW_DUTY_US 		(1550)
#define MAX_MOTOR_BW_DUTY_US 		(1445)
#define MIN_MOTOR_BW_DUTY_US 		(1200)

/*Pwm settings that are shared between servo and dc motor */
#define LEDC_Motor_And_Servo_MODE  	LEDC_HIGH_SPEED_MODE
#define LEDC_Motor_And_Servo_FREQ  	50   // 50Hz

/* Servo functions */
void init_servo_pwm();
void update_servo_pwm(int pulse_width_us);
void changeSTEER(int value);

/* Motor functions */
#define QUEUE_SIZE_SPEED 		   20
#define QUEUE_SIZE_DATATYPE_SPEED (sizeof(int))

void init_motor_pwm();
void carControl_calibrate_motor();
void update_motor_pwm(unsigned int pulse_width_us);
void changeMotorSpeed(int value);

/*Encoder*/
#define QUEUE_SIZE_ENCODER_PULSE 		   20
#define QUEUE_SIZE_DATATYPE_ENCODER_PULSE (sizeof(int))
#define PCNT_HIGH_LIMIT_WATCHPOINT 		   4 // at every 6 pulses forward the car has moved half a wheel and the callback is called
#define PCNT_LOW_LIMIT_WATCHPOINT  		  -4// at every 6 pulses backward the car has moved half a wheel and the callback is called
#define encoderGPIO_A 			    	   35
#define encoderGPIO_B 					   14
bool example_pcnt_on_reach(pcnt_unit_handle_t unit,const pcnt_watch_event_data_t *edata, void *user_ctx);
void configureEncoderInterrupts();

/*Car control*/
#define QUEUE_SIZE_CAR_COMMANDS 20
#define QUEUE_SIZE_DATATYPE_CAR_COMMANDS (sizeof(char*))

/*COMMANDS RECEIVED FROM APP*/
typedef enum {
    StopReceived = 0,      		// 00
    ForwardReceived = 1,        // 01
    BackwardReceived = 2,       // 02 //03 ->left light, 04 -> right light
    SpeedReceived = 5,          // 05
    SteerReceived = 6,          // 06
	AutonomousReceived = 7,		// 07
    PID_Changed = 8
} ReceivedState_app;

typedef struct {
    ReceivedState_app command;
    int command_value;
    bool has_value; // Indicates whether command_value is valid
    float KP;
    float KI;
    float KD;
} CarCommand;

/*Car control tasks*/
void carControl_Task(void *pvParameters);
void steer_task(void *pvParameters);

/*Car control functions*/
void carControl_init();
void carControl_Backward_init(int break_value);
CarCommand parseCommand(const char* commandStr);



#endif
