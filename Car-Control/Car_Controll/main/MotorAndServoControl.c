#include "MotorAndServoControl.h"
/*Include Network.h to send back to app diagnostics from the car*/
#include "Network.h"

//Queue set which adds all data from all queues.
QueueSetHandle_t QueueSetGeneralCommands;

// Initialize PWM for SERVO
void init_servo_pwm() {
	// Prepare and set configuration of timers
	ledc_timer_config_t ledc_timer = { .speed_mode = LEDC_Motor_And_Servo_MODE,
			.duty_resolution = LEDC_SERVO_DUTY_RESOLUTION, .timer_num =
			LEDC_SERVO_TIMER, .freq_hz = LEDC_Motor_And_Servo_FREQ, };
	ledc_timer_config(&ledc_timer);

	// Prepare individual configuration
	ledc_channel_config_t ledc_channel = { .channel = LEDC_SERVO_CH0_CHANNEL,
			.duty = 0, .gpio_num = LEDC_SERVO_GPIO, .speed_mode =
			LEDC_Motor_And_Servo_MODE, .timer_sel = LEDC_SERVO_TIMER, };
	ledc_channel_config(&ledc_channel);
}

// Update the PWM duty cycle for SERVO
void update_servo_pwm(int pulse_width_us) {
	uint32_t duty = (pulse_width_us * ((1 << LEDC_SERVO_DUTY_RESOLUTION) - 1))
			/ 20000;
	ledc_set_duty(LEDC_Motor_And_Servo_MODE, LEDC_SERVO_CH0_CHANNEL, duty);
	ledc_update_duty(LEDC_Motor_And_Servo_MODE, LEDC_SERVO_CH0_CHANNEL);
}

void changeSTEER(int value) {
	// Check if the value is within the expected range

	if (value < 0) {
		value = 0;
	} else if (value > 100) {
		value = 100;
	}
	// Scale the value to fit between 1000 and 2000
	int pulse_width_us = MIN_SERVO_DUTY_US + (value * (MAX_SERVO_DUTY_US - MIN_SERVO_DUTY_US) / 100);
	// ESP_LOGI("", "pwm_us %d", pulse_width_us);
	update_servo_pwm(pulse_width_us);
}

// Initialize PWM for Motor
void init_motor_pwm() {
	// Timer configuration, same as before
	ledc_timer_config_t ledc_timer = { .speed_mode = LEDC_Motor_And_Servo_MODE,
			.duty_resolution = LEDC_MOTOR_DUTY_RESOLUTION, .timer_num =
			LEDC_MOTOR_TIMER,  // Different timer for motor
			.freq_hz = LEDC_Motor_And_Servo_FREQ, };
	ledc_timer_config(&ledc_timer);

	// Channel configuration, similar to steering but different channel and GPIO
	ledc_channel_config_t ledc_channel = { .channel = LEDC_MOTOR_CH1_CHANNEL, // Different channel for motor
			.duty = 0, .gpio_num = LEDC_MOTOR_GPIO,  // Different GPIO for motor
			.speed_mode = LEDC_Motor_And_Servo_MODE, .timer_sel =
			LEDC_MOTOR_TIMER,    // Different timer for motor
			};
	ledc_channel_config(&ledc_channel);
}

// Update the PWM for Motor, similar to update_servo_pwm function
void update_motor_pwm(unsigned int pulse_width_us) {
	uint32_t duty = (pulse_width_us * ((1 << LEDC_MOTOR_DUTY_RESOLUTION) - 1))
			/ 20000;
	ledc_set_duty(LEDC_Motor_And_Servo_MODE, LEDC_MOTOR_CH1_CHANNEL, duty);
	ledc_update_duty(LEDC_Motor_And_Servo_MODE, LEDC_MOTOR_CH1_CHANNEL);
}

/* Change Motor Speed, similar to changeSTEER function */
void changeMotorSpeed(int value) {
	int pulse_width_us;
	//ESP_LOGI("", "value %d", value);
	if(value > 100)
		value = 100;
	else if(value < -100)
		value = -100;

	if (value == 0)
		pulse_width_us = 1500;
	else if (value >= 1) { // FORWARD
		// Forward mode: Scale between 1545 and 1700
		pulse_width_us = MIN_MOTOR_FW_DUTY_US + (value * (MAX_MOTOR_FW_DUTY_US - MIN_MOTOR_FW_DUTY_US) / 100);
		//ESP_LOGI("", "pwm_us %d", pulse_width_us);
	} else { 			// BACKWARD
		pulse_width_us = MAX_MOTOR_BW_DUTY_US - ((-value) * (MAX_MOTOR_BW_DUTY_US - MIN_MOTOR_BW_DUTY_US) / 100);
		//ESP_LOGI("", "pwm_us %d", pulse_width_us);
	}
	update_motor_pwm(pulse_width_us);
}

/* Encoder */
QueueHandle_t pulse_encoderQueue;
pcnt_unit_handle_t pcnt_unit = NULL;

bool example_pcnt_on_reach(pcnt_unit_handle_t unit,
		const pcnt_watch_event_data_t *edata, void *user_ctx) {
	BaseType_t high_task_wakeup;
	QueueHandle_t queue = (QueueHandle_t) user_ctx;
	// send event data to queue, from this interrupt callback
	xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	return (high_task_wakeup == pdTRUE);
}
void configureEncoderInterrupts() {

	pcnt_unit_config_t unit_config = { .high_limit = PCNT_HIGH_LIMIT_WATCHPOINT,
			.low_limit = PCNT_LOW_LIMIT_WATCHPOINT, };

	ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

	pcnt_glitch_filter_config_t filter_config = { // debouncer, lower value ==> higher the chance to act as low pass filter.
			.max_glitch_ns = 1000, };
	ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

	ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, PCNT_HIGH_LIMIT_WATCHPOINT)); // watch point which will trigger the callback function when
	ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, PCNT_LOW_LIMIT_WATCHPOINT)); // a pulse is generated

	pcnt_chan_config_t chan_a_config = { .edge_gpio_num = encoderGPIO_B,
			.level_gpio_num = encoderGPIO_A, };
	pcnt_channel_handle_t pcnt_chan_a = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

	pcnt_chan_config_t chan_b_config = { .edge_gpio_num = encoderGPIO_A,
			.level_gpio_num = encoderGPIO_B, };
	pcnt_channel_handle_t pcnt_chan_b = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

	ESP_ERROR_CHECK(
			pcnt_channel_set_edge_action(pcnt_chan_a,
					PCNT_CHANNEL_EDGE_ACTION_DECREASE,
					PCNT_CHANNEL_EDGE_ACTION_INCREASE));
	ESP_ERROR_CHECK(
			pcnt_channel_set_level_action(pcnt_chan_a,
					PCNT_CHANNEL_LEVEL_ACTION_KEEP,
					PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
	ESP_ERROR_CHECK(
			pcnt_channel_set_edge_action(pcnt_chan_b,
					PCNT_CHANNEL_EDGE_ACTION_INCREASE,
					PCNT_CHANNEL_EDGE_ACTION_DECREASE));
	ESP_ERROR_CHECK(
			pcnt_channel_set_level_action(pcnt_chan_b,
					PCNT_CHANNEL_LEVEL_ACTION_KEEP,
					PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

	pcnt_event_callbacks_t cbs = { .on_reach = example_pcnt_on_reach, };

	ESP_ERROR_CHECK(
			pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, pulse_encoderQueue));
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

CarCommand parseCommand(const char *commandStr)
{
	CarCommand cmd;
	memset(&cmd, 0, sizeof(cmd)); // Initialize the structure with zeros
	if (strncmp(commandStr, "08", 2) == 0)
	{
		// Handle command "08" specifically to parse KP, KI, KD values
		sscanf(commandStr, "08%f %f %f", &cmd.KP, &cmd.KI, &cmd.KD);
		cmd.command = PID_Changed; // Set the command to the appropriate enum value
		cmd.has_value = false;	   // Since KP, KI, KD are used, command_value is not relevant
	}
	else if (strlen(commandStr) >= 2)
	{
		char commandPart[3] = {commandStr[0], commandStr[1], '\0'};
		cmd.command = atoi(commandPart);
		// Check if there's more to the command than just the command part
		if (strlen(commandStr) > 2)
		{
			cmd.has_value = true;
			// Extract the command value, which is optional
			char valuePart[5];					   // Assuming the value part will not exceed 4 digits
			strncpy(valuePart, commandStr + 2, 4); // Copy the rest of the string as command value
			valuePart[4] = '\0';				   // Null-terminate the string
			cmd.command_value = atoi(valuePart);
		}
		else
		{
			cmd.has_value = false;
		}
	}
	return cmd;
}

void carControl_Backward_init()
{
	changeMotorSpeed(0);
	vTaskDelay(pdMS_TO_TICKS(5));
	changeMotorSpeed(-80);
	vTaskDelay(pdMS_TO_TICKS(5));
	changeMotorSpeed(0);
	vTaskDelay(pdMS_TO_TICKS(5));
}


bool calib_motor_done = false;
void carControl_calibrate_motor()
{
	update_motor_pwm(2000);
	vTaskDelay(pdMS_TO_TICKS(3000));
	update_motor_pwm(1000);
	vTaskDelay(pdMS_TO_TICKS(3000));
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(3000));
	ESP_LOGI(" ", "ESC Calibration over.");
	calib_motor_done = true;
}

QueueHandle_t diagnosticModeControlQueue = NULL;
QueueHandle_t autonomousModeControlQueue = NULL;
QueueHandle_t speed_commandQueue = NULL;
QueueHandle_t steer_commandQueue = NULL;
QueueHandle_t PID_commandQueue = NULL;
QueueSetHandle_t QueueSetAutonomousOrDiagnostic = NULL;

/* This variable allows the uart_task to fill Autonomous Queue for controlling the car. */
bool AutonomousMode = false;
void carControl_Task(void *pvParameters) {
	CarCommand cmd = { StopReceived, 0, false, 0.0f, 0.0f, 0.0f };
	int last_motor_speed = 0;
	int speed_multiplier = 0;
	int speed = 0;
	char *command;
	QueueSetMemberHandle_t xActivatedModeAorM_Member;

	while (1) {
		if (diagnosticModeControlQueue != NULL && autonomousModeControlQueue != NULL) {
			xActivatedModeAorM_Member = xQueueSelectFromSet(QueueSetAutonomousOrDiagnostic, portMAX_DELAY);

			if(xActivatedModeAorM_Member == diagnosticModeControlQueue)
			{
				AutonomousMode = false;
				xQueueReceive(xActivatedModeAorM_Member, &command, 0);
				cmd = parseCommand(command);
			}
			else if(xActivatedModeAorM_Member == autonomousModeControlQueue)
			{
				xQueueReceive(xActivatedModeAorM_Member, &cmd, 0);
			}


				//ESP_LOGI(" ", "%d %d", cmd.command, cmd.command_value);
				switch (cmd.command) {
				case StopReceived:
					speed =0;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					speed_multiplier = 0;
					break;
				case ForwardReceived:
					//ESP_LOGI(" ", "ForwardReceived");
					speed_multiplier = 1;
					speed = speed_multiplier * last_motor_speed;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					HLD_SendMessage("OKFWD!");
					break;
				case BackwardReceived:
					//ESP_LOGI(" ", "BackwardReceived");
					speed_multiplier = -1;
					speed = speed_multiplier * last_motor_speed;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					HLD_SendMessage("OKBWD!");
					break;
				case SpeedReceived:
					speed = speed_multiplier * cmd.command_value;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					last_motor_speed = cmd.command_value;
					//ESP_LOGI(" ", "SpeedReceived %d", speed_multiplier);
					HLD_SendMessage("OKSPEED!");
					break;
				case SteerReceived:
					//changeSTEER(cmd.command_value);
					xQueueSend(steer_commandQueue,&cmd.command_value,portMAX_DELAY);
					HLD_SendMessage("OKSTEER!");
					break;
				case AutonomousReceived:
					//ESP_LOGI(" ", "AutonomousReceived");
					AutonomousMode = true;
					break;
				case PID_Changed:
					xQueueSend(PID_commandQueue,&cmd,portMAX_DELAY);
					HLD_SendMessage("OKPID!");
					break;
				}

		} else
			vTaskDelay(pdMS_TO_TICKS(250));
	}
}

void steer_task(void *pvParameters) {
	int steer = 0;
	while (1) {
		if (steer_commandQueue != NULL) {
			if (xQueueReceive(steer_commandQueue, &steer, portMAX_DELAY) == pdPASS) {
				changeSTEER(steer);
			}
		} else
			vTaskDelay(pdMS_TO_TICKS(250));
	}
}

void carControl_init() {

	init_servo_pwm();
	init_motor_pwm();
	update_servo_pwm(1500); // ESC init
	update_motor_pwm(1500);
	//carControl_calibrate_motor();
	//while(calib_motor_done!=true) vTaskDelay(pdMS_TO_TICKS(50));
	vTaskDelay(pdMS_TO_TICKS(500)); // wait for init to complete
	xTaskCreatePinnedToCore(carControl_Task, "carControl_task", 4096, NULL, 6,
	NULL, 0U);
	xTaskCreatePinnedToCore(steer_task, "steer_task", 4096, NULL, 5,NULL, 1U);

}

