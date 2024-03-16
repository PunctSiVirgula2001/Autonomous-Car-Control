#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "esp_log.h"
#include "MotorAndServoControl.h"

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

int mapValue_pid(int value);
int reverseMapValue_pid(int input_value);
void setPIDParameters(); // set parameters through keyboard
void PIDTask(void *pvParameters);
