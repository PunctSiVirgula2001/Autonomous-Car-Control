#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "esp_log.h"
#include "MotorAndServoControl.h"

/*PID settings*/
#define PID_MAX_WINDUP 5 // to be tested
#define PID_D_TERM_FILTER_COEFFICIENT 0.5 // to be tested
#define PID_OUT_LIMIT_RATE_CHANGE 5 // represents the difference between outputs
// PID Structure
typedef struct {
    int setpoint;     // Desired value
    int measured;     // Current value
    float previous_error;
    float integral;
    float Kp;           // Proportional gain
    float Ki;           // Integral gain
    float Kd;           // Derivative gain
    float P_term;
    float I_term;
    float D_term;
    int Output;
} PID_t;

// PID Initialization
void PID_Init(PID_t *pid, float Kp, float Ki, float Kd);
// Compute PID
void PID_Compute(PID_t *pid);
// PID update params
void PID_UpdateParams(PID_t *pid, float new_Kp, float new_Ki, float new_Kd);
//PID Task
void PIDTask(void *pvParameters);

/*Sliding mean average settings*/
#define WINDOW_SIZE_SMA 8
double Sliding_Mean_Average(int newValue);
