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

// function for reading from keyboard --> TODO: Replace it by implementing PID setting from app.
void setPIDParameters() {
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

/*SLIDING MEAN AVERAGE function*/
double buffer[WINDOW_SIZE_SMA] = {0.0};
double sum = 0.0;

// Function to add value to the buffer and update the sliding mean
double Sliding_Mean_Average(int newValue) {
	// To keep track of the oldest value's index in the circular buffer
	static int index = 0;
    // Subtract the oldest value from the sum
    sum -= buffer[index];
    // Add the new value to the buffer and to the sum
    buffer[index] = (double)newValue;
    sum += newValue;
    // Calculate the new mean
    double mean = sum / (double)WINDOW_SIZE_SMA;
    // Move the index to the next position (circularly)
    index = (index + 1) % WINDOW_SIZE_SMA;
    return mean;
}

/*CONFIG_FREERTOS_HZ is set to 1000, this means the operating system tick rate is 100 ticks per second. Each tick would
therefore represent 1 millisecond (since 1000 milliseconds / 1000 ticks = 1 millisecond per tick).
To convert a tick count to milliseconds, you can multiply the number of ticks by 1*/

extern QueueHandle_t queuePulseCnt;
extern QueueHandle_t speedQueue;
extern QueueSetHandle_t QueueSet;
TickType_t xLastWakeTime =0U;
TickType_t xNewWakeTime =0U;
uint32_t pulse_time_ms=0U;
void PIDTask(void *pvParameters)
{
	 int pulseCount = 0;
	 int speed = 0;
	 double SMA_frequency = 0;
	 QueueSetMemberHandle_t xActivatedMember;
	 //ESP_LOGI("PulseCounter", "Pulse Count");
	    while (1) {
	    	//ESP_LOGI("PulseCounter", "Pulse Count");
	    	xActivatedMember = xQueueSelectFromSet(QueueSet ,portMAX_DELAY);

	        if (xActivatedMember == queuePulseCnt)
	        {
	            // Process the pulse count here (e.g., log it)
	        	xQueueReceive( xActivatedMember, &pulseCount, 0 );
	        	xNewWakeTime = xTaskGetTickCount();
	        	pulse_time_ms =pdTICKS_TO_MS((xNewWakeTime - xLastWakeTime));
	        	SMA_frequency =1.0/(Sliding_Mean_Average(pulse_time_ms) / 1000.0); // convert frequency HZ
	            ESP_LOGI("PID", "Frequency: %f", SMA_frequency);
	            xLastWakeTime = xNewWakeTime;
	        }

	        if (xActivatedMember == speedQueue)
	        {
	        	xQueueReceive( xActivatedMember, &speed, 0 );
	        	changeMotorSpeed(speed);
	        	ESP_LOGI("PID", "Speed: %d", speed);
	        }

	    }
}

