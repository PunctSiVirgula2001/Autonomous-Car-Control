#include "PID.h"
#include "Network.h"
#include <math.h>

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
	clamp_float(&(pid->I_term), -40, 40);  // Ensure I_term is within bounds
	pid->D_term = pid->Kd * (error - pid->previous_error);
	pid->Output = pid->P_term + pid->I_term + pid->D_term;
	clamp_int(&(pid->Output), -100, 100);  // Ensure Output is within bounds
	pid->previous_error = error;

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
bool car_stays_stopped = false;
void PIDTask(void *pvParameters) {
	/*Init motor's PID structure*/
	PID_t motorPID;
	/* TODO: Add steerPID for a greater efficiency in car's control when it enters autonomous mode. */
	PID_t steerPID;

	PID_Init(&motorPID, 0, 0, 0);
	int pulse_direction = 0;
	int setpoint_speed = 0;
	double SMA_frequency = 0;
	TickType_t newTime_backward = 0U;
	TickType_t newTime_stop = 0U;

	/* xActivatedMember receives messages from different queues, for example:
	 * -> Encoder pulses: - each time a series of PCNT_HIGH_LIMIT_WATCHPOINT / PCNT_LOW_LIMIT_WATCHPOINT
	 * 					    pulses happen, pulse_encoderQueue will be filled with either -1 or +1 depening
	 * 					    on the direction of the wheel spin, which will be further used to time stamp the
	 * 					    moment when the car has performed the desired number of pulses.
	 * -> User's setpoint for speed: - each time the user sets the desired speed on the app, then the speed_commandQueue
	 * 								   queue will be filled with a value between -100 and 100, sign depending
	 * 								   on the state : Backward or Forward selected by the user.
	 * -> User's desired configuration for PID's constants: - fills PID_commandQueue everytime a change in KP, KD, KI in app. */
	QueueSetMemberHandle_t xActivatedMember;

	while (1) {
		//ESP_LOGI("PulseCounter", "Pulse Count");
		xActivatedMember = xQueueSelectFromSet(QueueSet, pdMS_TO_TICKS(100));
		if (xActivatedMember == pulse_encoderQueue)
		{
			xQueueReceive(xActivatedMember, &pulse_direction, 0);
			if (pulse_direction > 0)
				pulse_direction = 1;
			else if (pulse_direction < 0)
			{
				pulse_direction = -1;
				newTime_backward = xTaskGetTickCount();
			}

			/* Calculate time between each series of pulses and then convert it to HZ. */
			xNewWakeTime = xTaskGetTickCount();
			pulse_time_ms = pdTICKS_TO_MS((xNewWakeTime - xLastWakePulse));
			SMA_frequency = 1.0 / (Sliding_Mean_Average(pulse_time_ms) / 1000.0);
			xLastWakePulse = xNewWakeTime;

			/* Update the measured value. */
			motorPID.measured = pulse_direction * SMA_frequency; // MEASURED

			//ESP_LOGI("PID", "Frequency: %f", pulse_direction * SMA_frequency);
		}
		if (xActivatedMember == speed_commandQueue)
		{
			xQueueReceive(xActivatedMember, &setpoint_speed, 0);
			motorPID.setpoint = setpoint_speed;

			/* If the setpoint is set to 0 then update the Measured Value
			 * to 0 so it will not interfere with PID updates. */
			if (setpoint_speed == 0)
			{
				motorPID.measured = 0;
				newTime_stop = xTaskGetTickCount();
			}

			//ESP_LOGI("PID", "Speed Setpoint: %d", setpoint_speed);
		}
		if (xActivatedMember == PID_commandQueue) {
			xQueueReceive(xActivatedMember, &onlyPIDValues, 0);
			PID_UpdateParams(&motorPID, onlyPIDValues.KP, onlyPIDValues.KI,
					onlyPIDValues.KD);
		}

		/* IN CASE OF CONFUSION:  -> Setpoint is given by the user, so only the Setpoint can
		 * 						     decide if the car should go backwards or not.
		 * 						  -> Output value if it goes below 0, it shall not bring the car
		 * 						  	 in a backward mode because it wasn't decided by the user.	*/



		/* Each time a pulse has happened, the task will be notified
		 * and PID will compute next OUTPUT value. */
		PID_Compute(&motorPID);

		/* If setpoint is set to 0 and the car is still moving, then BRAKE. */
		if(motorPID.setpoint == 0 && pulse_direction == 0)
		{
			/* Used a value above 0 because if I did 0, it will init the backward mode. */
			static int confirm_not_backward = 0;
			if(confirm_not_backward == 0)
			{
				motorPID.Output = 3;
				confirm_not_backward = 1;
			}
			/* Output is set to 0 only when it is known that the backward state is not initiated. */
			else if (confirm_not_backward == 1)
			{
				motorPID.Output = 0;
				confirm_not_backward = 0;
			}
		}

		/* If the setpoint is crossing the backward moving zone, brake. */

		//managePIDOutput(&motorPID);
		PID_backward_if_detected(&motorPID);
		changeMotorSpeed(motorPID.Output);

		/*If the time between pulses is too long set the measured value to 0.*/
		if (xTaskGetTickCount()-xLastWakePulse > pdMS_TO_TICKS(700))
		{
			motorPID.measured = 0;
			if(motorPID.setpoint == 0)
				motorPID.I_term = 0;
		}


		int abs_measured = abs(motorPID.measured);
		float abs_I_term = fabs((double)motorPID.I_term);
		sendCommandApp(MEASURED_VALUE, (int*)&abs_measured, INT);
		sendCommandApp(I_TERM_VALUE, (float*)&abs_I_term, FLOAT);

		//ESP_LOGI("SP", "Output: %d , P_term: %f, I_term: %f, D_term: %f \n",
		//		myPID.Output, myPID.P_term, myPID.I_term, myPID.D_term);
		//ESP_LOGI("MS", "Current time: %f \n", (float)pdTICKS_TO_MS(xTaskGetTickCount()));
	}
}

/*Clamp function for PID*/
void clamp_float(float *value, float min, float max) {
	if (*value > max) {
		*value = max;
	} else if (*value < min) {
		*value = min;
	}
}

void clamp_int(int *value, int min, int max) {
	if (*value > max) {
		*value = max;
	} else if (*value < min) {
		*value = min;
	}
}

/*PID motor backward init*/
void PID_backward_if_detected(PID_t *motor) {
    static int backward_active = 0;

    if (motor->setpoint < 0 && motor->measured >= 0) {
        if (!backward_active || motor->Output < 0) {
            carControl_Backward_init();
            motor->I_term = 0;  // Reset integral term
            backward_active = 1;
        }
    } else if (motor->setpoint >= 0) {
        backward_active = 0;  // Reset backward state only when well above the backward threshold
    }
}

/*START PID TASK function*/

void start_PID_task()
{
	xTaskCreatePinnedToCore(PIDTask, "PIDTask", 4096, NULL, 10, NULL, 1U);
}
