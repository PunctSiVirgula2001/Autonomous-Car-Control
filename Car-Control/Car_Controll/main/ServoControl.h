#ifndef SERVOCONTROL
#define SERVOCONTROL

#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_task_wdt.h"

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (27)  // Change this to your GPIO pin
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_FREQ             50   // 50Hz
#define LEDC_DUTY_RESOLUTION  LEDC_TIMER_10_BIT // Resolution of 10 bits


void init_pwm();
void update_pwm(int pulse_width_us);
void changeSTEER(int value);

////////

#define LEDC_MOTOR_TIMER LEDC_TIMER_1
#define LEDC_MOTOR_CHANNEL LEDC_CHANNEL_1
#define LEDC_MOTOR_GPIO 4  // Replace with the actual GPIO number you are using



void init_motor_pwm();
void update_motor_pwm(unsigned int pulse_width_us);
void changeMotorSpeed(int value);

#endif
