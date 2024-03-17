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

// PID Structure
typedef struct {
    int setpoint;     // Desired value
    int measured;     // Current value
    float previous_error;
    float integral;
    float Kp;           // Proportional gain
    float Ki;           // Integral gain
    float Kd;           // Derivative gain
} PID_t;

// PID Initialization
void PID_Init(PID_t *pid, float Kp, float Ki, float Kd);
// Compute PID
float PID_Compute(PID_t *pid, float deltaTime) ;
// set parameters through keyboard
void setPIDParameters();
void PIDTask(void *pvParameters);

/*Sliding mean average settings*/
#define WINDOW_SIZE_SMA 8
double Sliding_Mean_Average(int newValue);
