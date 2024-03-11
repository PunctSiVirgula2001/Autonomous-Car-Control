#ifndef SERVOCONTROL
#define SERVOCONTROL

#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_task_wdt.h"
#include "driver/pulse_cnt.h"
#include <stdbool.h> // For bool type
#include <stdlib.h> // For atoi function
#include <string.h> // For strlen function


/*Settings for LEDC */
#define LEDC_SERVO_TIMER          	LEDC_TIMER_0
#define LEDC_SERVO_GPIO       	  	(27)
#define LEDC_SERVO_CH0_CHANNEL    	LEDC_CHANNEL_0
#define LEDC_SERVO_DUTY_RESOLUTION  LEDC_TIMER_10_BIT

#define LEDC_MOTOR_TIMER 			LEDC_TIMER_1
#define LEDC_MOTOR_CH1_CHANNEL 		LEDC_CHANNEL_1
#define LEDC_MOTOR_GPIO 			(4)
#define LEDC_MOTOR_DUTY_RESOLUTION 	LEDC_TIMER_16_BIT

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
#define EXAMPLE_PCNT_HIGH_LIMIT 2000000000
#define EXAMPLE_PCNT_LOW_LIMIT  -2000000000
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
bool continuousActionRequired(int command);


#endif
