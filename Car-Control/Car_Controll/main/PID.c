#include "PID.h"

// PID Initialization
void PID_Init(PID_t *pid, float Kp, float Ki, float Kd) {
	pid->setpoint = 0;
	pid->measured = 0;
	pid->previous_error = 0;
	pid->integral = 0;
	pid->Kp = Kp;
	pid->Ki = Ki;
	pid->Kd = Kd;
}

// Compute PID
float PID_Compute(PID_t *pid, float deltaTime) {
	float error = pid->setpoint - pid->measured;
	pid->integral += error * deltaTime; // Integral is accumulated error over time
	float derivative = (error - pid->previous_error) / deltaTime;
	float output = pid->Kp * error + pid->Ki * pid->integral
			+ pid->Kd * derivative;
	pid->previous_error = error;

	return output; // This is the manipulated variable (e.g., motor speed, heater power)
}

int mapValue_pid(int input_value) { //200 -----> 1024
	int min_source = 0;
	int max_source = 200;
	int min_target = -1024;
	int max_target = 1024;

	int mapped_value = min_target
			+ ((input_value - min_source) * (max_target - min_target))
					/ (max_source - min_source);

	return mapped_value;
}

int reverseMapValue_pid(int input_value) { //1024 ---> 200
	int min_source = -1024;
	int max_source = 1024;
	int min_target = 0;
	int max_target = 200;

	int reverse_mapped_value = min_target
			+ ((input_value - min_source) * (max_target - min_target))
					/ (max_source - min_source);

	return reverse_mapped_value;
}

//void app_main() {
//    PID_t motorPID;
//
//    // Initialize PID with some sample values
//    PID_Init(&motorPID, 1.0, 0.1, 0.01);
//
//    // Set your desired value
//    motorPID.setpoint = 100.0;
//
//    while (1) {
//        // Update your measured value here
//        motorPID.measured = get_measured_value(); // Replace with your function to get the current measured value
//
//        float motorSpeed = PID_Compute(&motorPID);
//
//        // Apply the computed PID value (motorSpeed) to your system
//        set_motor_speed(motorSpeed); // Replace with your function to set the motor speed
//
//        vTaskDelay(pdMS_TO_TICKS(10));  // Sample every 10ms, adjust as needed
//    }
//}

