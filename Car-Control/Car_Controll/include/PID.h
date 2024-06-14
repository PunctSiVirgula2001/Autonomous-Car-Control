#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "esp_log.h"
#include "MotorAndServoControl.h"

/* PID settings */
#define PID_MAX_WINDUP 5 // Maximum windup value for the PID controller (to be tested)
#define PID_D_TERM_FILTER_COEFFICIENT 0.5 // Coefficient for filtering the derivative term (to be tested)
#define PID_OUT_LIMIT_RATE_CHANGE 5 // Maximum rate of change for the PID output (represents the difference between outputs)

// PID Structure
typedef struct {
    int setpoint;     // Desired value
    int measured;     // Current value
    float previous_error; // Previous error value
    float integral;   // Integral term
    float Kp;         // Proportional gain
    float Ki;         // Integral gain
    float Kd;         // Derivative gain
    float P_term;     // Proportional term
    float I_term;     // Integral term
    float D_term;     // Derivative term
    int Output;       // PID output
} PID_t;

// PID Initialization
void PID_Init(PID_t *pid, float Kp, float Ki, float Kd);

// Compute PID
void PID_Compute(PID_t *pid);

/* PID update params */
void PID_UpdateParams(PID_t *pid, float new_Kp, float new_Ki, float new_Kd);

/* PID Task */
void PIDTask(void *pvParameters);

/* Sliding mean average settings */
#define WINDOW_SIZE_SMA 10 // Window size for sliding mean average

/* Sliding Mean Average function */
double Sliding_Mean_Average(int newValue);

/*Clamp function for PID - types*/
void clamp_float(float *value, float min, float max);
void clamp_int(int *value, int min, int max);

/*START PID TASK function*/
void start_PID_task();

/* Tick hook for continuosly updating car's speed to 0 when's needed. */
void speedCheckTickHook(void);
void init_speedCheckTickHook(void);
