#include "PID.h"

//PID
PID_t motorPID;
float deltaTime = 0.02;  // 20 ms = 0.02 seconds

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

	return output; // This is the manipulated variable (e.g., motor speed, servo speed/control)
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

void setPIDParameters() { // function for reading from keyboard
	char buffer[256];

	if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
		char *command = strtok(buffer, " ");
		if (strcmp(command, "set") == 0) {
			float kp = atof(strtok(NULL, " "));
			float ki = atof(strtok(NULL, " "));
			float kd = atof(strtok(NULL, " "));

			// Update the PID parameters
			motorPID.Kp = kp;
			motorPID.Ki = ki;
			motorPID.Kd = kd;

			printf("PID parameters updated: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", kp, ki, kd);
		}
	}
}

extern QueueHandle_t queuePulseCnt;

void PIDTask(void *pvParameters)
{
	 int pulseCount=0;
	 ESP_LOGI("PulseCounter", "Pulse Count");
	    while (1) {
	    	ESP_LOGI("PulseCounter", "Pulse Count");
	        if (xQueueReceive(queuePulseCnt, &pulseCount, portMAX_DELAY)) {
	            // Process the pulse count here (e.g., log it)
	            ESP_LOGI("PulseCounter", "Pulse Count: %d", pulseCount);
	        }
	        else {
	        	ESP_LOGI("PulseCounter", "Pulse Count");
	        	vTaskDelay(pdMS_TO_TICKS(50));
	        }

	    }
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

