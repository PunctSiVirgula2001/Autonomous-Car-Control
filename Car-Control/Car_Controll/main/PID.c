#include "PID.h"
#include "Network.h"

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
	pid->P_term = 0;
	pid->I_term = 0;
	pid->D_term = 0;
}

// Compute PID
void PID_Compute(PID_t *pid) {
	float error = pid->setpoint - pid->measured;
	pid->P_term = pid->Kp * error;
	pid->I_term += (pid->Ki * error);
	pid->D_term = pid->Kd * (error - pid->previous_error);
	pid->Output = pid->P_term + pid->I_term + pid->D_term;
	pid->previous_error = error;
	if (pid->I_term >= 100)
		pid->I_term = 100;
	else if (pid->I_term <= -100)
		pid->I_term = -100;

	if (pid->Output >= 100)
		pid->Output = 100;
	else if (pid->Output <= -100)
		pid->Output = -100;

	//TODO: Reset the I_term once in a while.
	// ... and other necessary computations ...
}

void PID_UpdateParams(PID_t *pid, float new_Kp, float new_Ki, float new_Kd) {
	// Optionally lock the task if needed to prevent concurrent access
	// taskENTER_CRITICAL();

	// Update the PID coefficients
	pid->Kp = new_Kp;
	pid->Ki = new_Ki;
	pid->Kd = new_Kd;

	// Reinitialize any error sums or stateful terms if necessary
	pid->I_term = 0; // Reset integral term if needed
	pid->previous_error = 0; // Reset the previous error if the derivative term should be reinitialized

	// Optionally include anti-windup logic or re-initialize other elements here

	// Optionally unlock the task if it was previously locked
	// taskEXIT_CRITICAL();
}

/*SLIDING MEAN AVERAGE function*/
double buffer[WINDOW_SIZE_SMA] = { 0.0 };
double sum = 0.0;

// Function to add value to the buffer and update the sliding mean
double Sliding_Mean_Average(int newValue) {
	// To keep track of the oldest value's index in the circular buffer
	static int index = 0;
	// Subtract the oldest value from the sum
	sum -= buffer[index];
	// Add the new value to the buffer and to the sum
	buffer[index] = (double) newValue;
	sum += newValue;
	// Calculate the new mean
	double mean = sum / (double) WINDOW_SIZE_SMA;
	// Move the index to the next position (circularly)
	index = (index + 1) % WINDOW_SIZE_SMA;
	return mean;
}

/*CONFIG_FREERTOS_HZ is set to 1000, this means the operating system tick rate is 100 ticks per second. Each tick would
 therefore represent 1 millisecond (since 1000 milliseconds / 1000 ticks = 1 millisecond per tick).
 To convert a tick count to milliseconds, you can multiply the number of ticks by 1*/

extern QueueHandle_t pulse_encoderQueue;
extern QueueHandle_t speed_commandQueue;
extern QueueHandle_t PID_commandQueue;
extern QueueSetHandle_t QueueSet;
TickType_t xLastWakePulse = 0U;
TickType_t xNewWakeTime = 0U;
TickType_t pulse_time_ms = 0U;
CarCommand onlyPIDValues;
TickType_t startInitialSendingTime = 0U;
void PIDTask(void *pvParameters) {
	/*Init PID structure*/
	PID_t myPID;
	PID_Init(&myPID, 0, 0, 0);
	int pulse_direction = 0;
	int setpoint_speed = 0;
	double SMA_frequency = 0;
	TickType_t newTime_backward = 0U;
	TickType_t newTime_stop = 0U;
	//TickType_t measured_value_delay = xTaskGetTickCount();
	QueueSetMemberHandle_t xActivatedMember;
	//ESP_LOGI("PulseCounter", "Pulse Count");
	while (1) {
		//ESP_LOGI("PulseCounter", "Pulse Count");

		xActivatedMember = xQueueSelectFromSet(QueueSet, pdMS_TO_TICKS(500));

		if (xActivatedMember == pulse_encoderQueue) {
			// Process the pulse count here (e.g., log it)
			xQueueReceive(xActivatedMember, &pulse_direction, 0);
			if (pulse_direction > 0)
				pulse_direction = 1;
			else if (pulse_direction < 0) {
				pulse_direction = -1;
				//set_backward = true;
				newTime_backward = xTaskGetTickCount();
			}

			xNewWakeTime = xTaskGetTickCount();
			pulse_time_ms = pdTICKS_TO_MS((xNewWakeTime - xLastWakePulse));
			SMA_frequency = 1.0
					/ (Sliding_Mean_Average(pulse_time_ms) / 1000.0); // convert frequency HZ
			ESP_LOGI("PID", "Frequency: %f", pulse_direction * SMA_frequency);
			xLastWakePulse = xNewWakeTime;
			myPID.measured = pulse_direction * SMA_frequency;
		}
		if (xActivatedMember == speed_commandQueue) {
			xQueueReceive(xActivatedMember, &setpoint_speed, 0);
			//changeMotorSpeed(setpoint_speed);
			myPID.setpoint = setpoint_speed;
			if (setpoint_speed == 0)
				newTime_stop = xTaskGetTickCount();
			//ESP_LOGI("PID", "Speed Setpoint: %d", setpoint_speed);
		}
		if (xActivatedMember == PID_commandQueue) {
			xQueueReceive(xActivatedMember, &onlyPIDValues, 0);
			PID_UpdateParams(&myPID, onlyPIDValues.KP, onlyPIDValues.KI,
					onlyPIDValues.KD);
		}
		PID_Compute(&myPID);
		if (myPID.Output < 0
			&& xTaskGetTickCount() - newTime_backward > pdMS_TO_TICKS(1000)
			&& setpoint_speed != 0) {
			carControl_Backward_init();
		}
		if (setpoint_speed == 0
			&& xTaskGetTickCount()-newTime_stop>pdMS_TO_TICKS(1500)) { // Put a final stop only after the command was received, so the PID would first break.
			myPID.Output = 0;
			myPID.measured = 0;
			pulse_direction = 0;
			myPID.I_term = 0;
		}
		if (xTaskGetTickCount()-xLastWakePulse > pdMS_TO_TICKS(500))
			myPID.measured = 0;
		changeMotorSpeed(myPID.Output);
		//TickType_t curentTime = pdTICKS_TO_MS(xTaskGetTickCount());
		sendCommandApp(MEASURED_VALUE, (int*)abs(myPID.measured));
		sendCommandApp(I_TERM_VALUE, (int*)abs((int)myPID.I_term));
		//ESP_LOGI("SP", "Output: %d , P_term: %f, I_term: %f, D_term: %f \n",
		//		myPID.Output, myPID.P_term, myPID.I_term, myPID.D_term);
		ESP_LOGI("MS", "Current time: %f \n", (float)pdTICKS_TO_MS(xTaskGetTickCount()));
	}
}

