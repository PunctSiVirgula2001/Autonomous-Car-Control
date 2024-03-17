#ifndef SERVOCONTROL
#define SERVOCONTROL

#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_task_wdt.h"
#include "driver/pulse_cnt.h"
#include <stdbool.h> // For bool type
#include <stdlib.h> // For atoi function
#include <string.h> // For strlen function


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
#define MAX_MOTOR_FW_DUTY_US 		(1600)
#define MIN_MOTOR_FW_DUTY_US 		(1560)
#define MAX_MOTOR_BW_DUTY_US 		(1440)
#define MIN_MOTOR_BW_DUTY_US 		(1300)

/*Pwm settings that are shared between servo and dc motor */
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

/*Encoder*/
#define PCNT_HIGH_LIMIT_WATCHPOINT 6
#define PCNT_LOW_LIMIT_WATCHPOINT  -6
#define encoderGPIO_A 35
#define encoderGPIO_B 14

bool example_pcnt_on_reach(pcnt_unit_handle_t unit,const pcnt_watch_event_data_t *edata, void *user_ctx);
void configureEncoderInterrupts();

/*Car control*/
/*COMMANDS RECEIVED FROM APP*/
typedef enum {
    StopReceived = 0,      		// 00
    ForwardReceived = 1,        // 01
    BackwardReceived = 2,       // 02
    SpeedReceived = 5,          // 05
    SteerReceived = 6,          // 06
	AutonomousReceived = 7		// 07
} ReceivedState_app;

typedef struct {
	ReceivedState_app command;
    int command_value;
    bool has_value; // Indicates whether command_value is valid
} CarCommand;


void carControl_Task(void *pvParameters);
void carControl_init();
CarCommand parseCommand(const char* commandStr);



#endif
