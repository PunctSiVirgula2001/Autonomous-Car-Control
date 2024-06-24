#include "I2C_common.h"
#include "PID.h"
#include "Network.h"
#include <math.h>

/*Init motor's PID structure*/
PID_t motorPID;
/* TODO: Add steerPID for a greater efficiency in car's control when it enters autonomous mode. */
PID_t steerPID;

bool sens_fw_has_obstacle = false;
bool sens_bw_has_obstacle = false;
extern int speed_distance_sens_scaling;


TickType_t xLastWakePulse = 0U;
TickType_t xNewWakeTime = 0U;
TickType_t pulse_time_ms = 0U;


void speedCheckTickHook(void) {
    static TickType_t old_time = 0U;
    static TickType_t new_time = 0U;
    static uint32_t unchanged_ticks = 0; // Counter for unchanged ticks

    new_time = xNewWakeTime;

    if (new_time == old_time) {
        unchanged_ticks++;
    } else {
        unchanged_ticks = 0;
        old_time = new_time;
    }

    if (unchanged_ticks >= 400) { // If speed hasn't changed for 200 ticks (1 second if tick rate is 1 ms)
        motorPID.measured = 0;     // Reset measured speed to 0
        if(motorPID.setpoint == 0)
        	motorPID.I_term = 0;
        unchanged_ticks = 0;       // Reset the unchanged ticks counter
    }

}


void init_speedCheckTickHook(void) {
    esp_err_t err;
    UBaseType_t cpuid = 1U; // Get the ID of the current core

    err = esp_register_freertos_tick_hook_for_cpu(speedCheckTickHook, cpuid);
    if (err != ESP_OK) {
        // Handle error
    }
}


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
#if low_pass_filter_derivative_chan == ON
    pid->previous_D_term = 0; // Add this to store the previous derivative term
    pid->alpha = 0;           // Smoothing factor for the low-pass filter
#endif
}

// Define constants at a global scope
const float primarySmoothingFactor = 0.1;  // Primary EMA smoothing factor
const float secondarySmoothingFactor = 0.05;  // Secondary EMA smoothing factor
const float maxChangePerCycle = 5.0;  // Maximum rate of output change per cycle

static float secondaryEMA = 0;  // Initialize secondary EMA output

// Compute PID with enhanced smoothing mechanisms
void PID_Compute(PID_t *pid, float dt) {
    float error = pid->setpoint - pid->measured;

    // Proportional term
    pid->P_term = pid->Kp * error;

    // Integral term
    pid->I_term += pid->Ki * error * dt;
    clamp_float(&(pid->I_term), -45, 45);  // Ensure I_term is within bounds

    #if low_pass_filter_derivative_chan == ON
        // Derivative term with low-pass filter
        float raw_D_term = error - pid->previous_error;
        pid->D_term = pid->alpha * pid->previous_D_term + (1 - pid->alpha) * raw_D_term / dt;
        pid->D_term *= pid->Kd;
    #else
        // Derivative term without low-pass filter
        pid->D_term = pid->Kd * (error - pid->previous_error) / dt;
    #endif

    // Calculate the proposed new output
    float newOutput = pid->P_term + pid->I_term + pid->D_term;

    // Apply primary Exponential Moving Average (EMA)
    float primaryEMAOutput = primarySmoothingFactor * newOutput + (1 - primarySmoothingFactor) * pid->previousOutput;

    // Apply secondary EMA for further smoothing
    secondaryEMA = secondarySmoothingFactor * primaryEMAOutput + (1 - secondarySmoothingFactor) * secondaryEMA;

    // Apply rate limiting to smooth large jumps
    float outputDifference = secondaryEMA - pid->previousOutput;
    if (fabs(outputDifference) > maxChangePerCycle) {
        if (outputDifference > 0) {
            pid->Output = pid->previousOutput + maxChangePerCycle;
        } else {
            pid->Output = pid->previousOutput - maxChangePerCycle;
        }
    } else {
        pid->Output = secondaryEMA;
    }

    // Ensure the output is within the allowed range
    clamp_int(&(pid->Output), -100, 100);  // Ensure Output is within bounds

    // Update previous values for next iteration
    pid->previousOutput = pid->Output;
    pid->previous_error = error;
    #if low_pass_filter_derivative_chan == ON
        pid->previous_D_term = raw_D_term;
    #endif
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

/* Speed scaler for determining the correct threshold for distance of stop. */
float get_speed_distance_sens_scaling(float speed, float speed_min, float speed_max) {
    if (speed <= speed_min) {
        return 1.0;
    } else if (speed >= speed_max) {
        return 5.0;
    } else {
        // Linear scaling between 1.0 and 5.0 for speeds between speed_min and speed_max
        float t = (speed - speed_min) / (speed_max - speed_min); // Normalized speed in range [0, 1]
        return 1.0 + t * (5.0 - 1.0); // Linear interpolation
    }
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

/*CONFIG_FREERTOS_HZ is set to 1000, this means the operating system tick rate is 1000 ticks per second. Each tick would
 therefore represent 1 millisecond (since 1000 milliseconds / 1000 ticks = 1 millisecond per tick).
 To convert a tick count to milliseconds, you can multiply the number of ticks by 1*/

extern QueueHandle_t pulse_encoderQueue;
extern QueueHandle_t speed_commandQueue;
extern QueueHandle_t PID_commandQueue;
extern QueueHandle_t I2C_commandQueue;
extern QueueSetHandle_t QueueSetPIDNecessaryCommands;
CarCommand onlyPIDValues;
I2C_COMMAND I2C_command;
double dt = 1000.0;
int old_setpoint;
TickType_t startInitialSendingTime = 0U;
bool car_stays_stopped = false;
void PIDTask(void *pvParameters) {

	PID_Init(&motorPID, 0, 0, 0);
	int pulse_direction = 0;
	int setpoint_speed = 0;
	double SMA_frequency = 0;

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

		xActivatedMember = xQueueSelectFromSet(QueueSetPIDNecessaryCommands, pdMS_TO_TICKS(100));
		if (xActivatedMember == pulse_encoderQueue) {
		    // Procesare pentru pulse_encoderQueue
		    xQueueReceive(xActivatedMember, &pulse_direction, 0);
		    if (pulse_direction > 0)
		        pulse_direction = 1;
		    else if (pulse_direction < 0)
		        pulse_direction = -1;

		    // Calculează timpul dintre impulsuri și convertește-l în Hz
		    xNewWakeTime = xTaskGetTickCount();
		    pulse_time_ms = pdTICKS_TO_MS((xNewWakeTime - xLastWakePulse));
		    dt = (Sliding_Mean_Average(pulse_time_ms) / 1000.0);
		    if (dt == 0) {
		        SMA_frequency = 0;  // Evită diviziunea cu zero
		    } else {
		        SMA_frequency = 1.0 / dt;
		    }
		    xLastWakePulse = xNewWakeTime;

		    // Actualizează valoarea măsurată
		    motorPID.measured = pulse_direction * SMA_frequency;
		    speed_distance_sens_scaling = get_speed_distance_sens_scaling(fabs((double)motorPID.measured), 5, 40);
		} else {
		    // Procesare pentru alte cozi active
		    TickType_t xCurrentTime = xTaskGetTickCount();
		    dt += pdTICKS_TO_MS((xCurrentTime - xLastWakePulse)) / 1000.0;
		    xLastWakePulse = xCurrentTime;

		    if (dt == 0) {
		        SMA_frequency = 0;  // Evită diviziunea cu zero
		    } else {
		        SMA_frequency = 1.0 / dt;
		    }

		    // Actualizează valoarea măsurată pentru alte cozi
		    motorPID.measured = pulse_direction * SMA_frequency;
		}
		if (xActivatedMember == speed_commandQueue)
		{
			xQueueReceive(xActivatedMember, &setpoint_speed, 0);
			motorPID.setpoint = setpoint_speed;
		}
		if (xActivatedMember == PID_commandQueue) {
			xQueueReceive(xActivatedMember, &onlyPIDValues, 0);
			PID_UpdateParams(&motorPID, onlyPIDValues.KP, onlyPIDValues.KI,
					onlyPIDValues.KD);
		}
		if(xActivatedMember == I2C_commandQueue)
		{

			xQueueReceive(xActivatedMember, &I2C_command, 0);
			if (I2C_command.sendingSensor == I2C_distance_sens_fw_dev_handle && I2C_command.command == I2C_STOP_MOTOR )
				sens_fw_has_obstacle = true;
			else  if(I2C_command.sendingSensor == I2C_distance_sens_fw_dev_handle && I2C_command.command == I2C_START_MOTOR)
			{
				sens_fw_has_obstacle = false;
			}
			if (I2C_command.sendingSensor == I2C_distance_sens_bw_handle && I2C_command.command == I2C_STOP_MOTOR )
				sens_bw_has_obstacle = true;
			else  if(I2C_command.sendingSensor == I2C_distance_sens_bw_handle && I2C_command.command == I2C_START_MOTOR)
			{
				sens_bw_has_obstacle = false;
			}
		}

		if((sens_fw_has_obstacle == true && motorPID.setpoint > 0) || (sens_bw_has_obstacle == true && motorPID.setpoint < 0))
		{
			old_setpoint = motorPID.setpoint;
			motorPID.setpoint = 0;
		}

		/* IN CASE OF CONFUSION:  -> Setpoint is given by the user, so only the Setpoint can
		 * 						     decide if the car should go backwards or not.
		 * 						  -> Output value if it goes below 0, it shall not bring the car
		 * 						  	 in a backward mode because it wasn't decided by the user.	*/

		/* Each time a pulse has happened, the task will be notified
		 * and PID will compute next OUTPUT value. */
		/* If the Setpoint is crossing the backward moving zone, brake. */


		/*If the time between pulses is too long set the measured value to 0.*/

		PID_Compute(&motorPID,dt);
		static bool backward = false;
		if(motorPID.Output < 0 && backward == false)
		{
			carControl_Backward_init(4*motorPID.Output);
			backward = true;
		}
		else if(motorPID.Output >= 0)
			backward = false;

		changeMotorSpeed(motorPID.Output);

		/* Send diagnostic data to phone app in order to monitor the speed and behaviour of Integral value. */

		int abs_measured = abs(motorPID.measured);
		float abs_I_term = fabs((double)motorPID.I_term);
		static int interrupt_counter = 0; // Contor static pentru a număra întreruperile

		/* Increment the interrupt counter */
		interrupt_counter++;

		/* Send only the data that is different from the one sent before, and at every 2 interrupts. */
		if (interrupt_counter % 2 == 0) {
		    sendCommandApp(MEASURED_VALUE, (int*)&abs_measured, INT);
		    sendCommandApp(I_TERM_VALUE, (float*)&abs_I_term, FLOAT);
		    /* Reset the interrupt counter after sending the data */
		    interrupt_counter = 0;
		}
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

/*START PID TASK function*/

void start_PID_task()
{
	init_speedCheckTickHook();
	xTaskCreatePinnedToCore(PIDTask, "PIDTask", 4096, NULL, 7, NULL, 1U);
}
