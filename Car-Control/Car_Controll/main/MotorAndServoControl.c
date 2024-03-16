#include "MotorAndServoControl.h"

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
	int scaled_value = 1300
			+ (int) (((1.0f - (100.0f - (float) value) / 100.0f) * 400.0f)); // scaled value will be between 1300 and 1300+400
	ESP_LOGI("", "pwm_us %d", scaled_value);
	update_servo_pwm(scaled_value);
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

// Change Motor Speed, similar to changeSTEER function
void changeMotorSpeed(int value) {
	int pulse_width_us;
	ESP_LOGI("", "value %d", value);
	if (value == 0)
		pulse_width_us = 1500;
	else if (value >= 1) { // FORWARD
		// Forward mode: Scale between 1545 and 1700
		pulse_width_us = 1545 + (value * (1700 - 1545) / 100);
		ESP_LOGI("", "pwm_us %d", pulse_width_us);
	} else { 			// BACKWARD
		pulse_width_us = 1455 - ((-value) * (1455 - 1240) / 100);
		ESP_LOGI("", "pwm_us %d", pulse_width_us);
	}
	update_motor_pwm(pulse_width_us);
}

/* Encoder */
QueueHandle_t queuePulseCnt;
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
	queuePulseCnt = xQueueCreate(10, sizeof(int));
	pcnt_unit_config_t unit_config = { .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
			.low_limit = EXAMPLE_PCNT_LOW_LIMIT, };

	ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

	pcnt_glitch_filter_config_t filter_config = { // debouncer
			.max_glitch_ns = 1000, };
	ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

	ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, 6)); // watch point which will trigger the callback function when
	ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, -6)); // a pulse is generated

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
			pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queuePulseCnt));
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
	//ESP_LOGI("PulseCounter", "Aici");

	// Report counter value
//	int event_count = 0;
//	while (1) {
//		if (xQueueReceive(queuePulseCnt, &event_count, pdMS_TO_TICKS(1000))) {
//			ESP_LOGI("", "Watch point event, count: %d", event_count);
//		}
//	}

}

CarCommand parseCommand(const char *commandStr) {
	CarCommand cmd;
	memset(&cmd, 0, sizeof(cmd)); // Initialize the structure with zeros

	if (strlen(commandStr) >= 2) {
		// Extract the command, which is always present
		char commandPart[3] = { commandStr[0], commandStr[1], '\0' };
		cmd.command = atoi(commandPart);

		// Check if there's more to the command than just the command part
		if (strlen(commandStr) > 2) {
			cmd.has_value = true;
			// Extract the command value, which is optional
			char valuePart[5]; // Assuming the value part will not exceed 4 digits
			strncpy(valuePart, commandStr + 2, 4); // Copy the rest of the string as command value
			valuePart[4] = '\0'; // Null-terminate the string
			cmd.command_value = atoi(valuePart);
		} else {
			cmd.has_value = false;
		}
	}
	return cmd;
}

//typedef enum {
//    StopReceived = 0,      		// 00
//    ForwardReceived = 1,        // 01
//    BackwardReceived = 2,       // 02
//    SpeedReceived = 5,          // 05
//    SteerReceived = 6,          // 06
//	AutonomousReceived = 7		// 07
//} ReceivedState_app;

QueueHandle_t carControlQueue = NULL;

void carControl_Task(void *pvParameters) {
	CarCommand cmd = { StopReceived, 0, false };
	int last_motor_speed = 0;
	int speed_multiplier = 0;
	char *command;
	while (1) {
		if (carControlQueue != NULL) {
			if (xQueueReceive(carControlQueue, &command,
			portMAX_DELAY) == pdPASS) {
				cmd = parseCommand(command);
				ESP_LOGI(" ", "%d %d", cmd.command, cmd.command_value);
				free(command);
				switch (cmd.command) {
				case StopReceived:
					changeMotorSpeed(0); // to be changed
					speed_multiplier = 0;
					// if the car is moving forward : logic for breaking.
					// if the car is moving backward : logic for breaking.
					// if the car is not moving but there are very small changes in speed(like pushing it a bit), don't apply any change
					break;
				case ForwardReceived:
					ESP_LOGI(" ", "ForwardReceived");
					speed_multiplier = 1;
					changeMotorSpeed(speed_multiplier * last_motor_speed); // to be replaced with notify task PID.
					break;
				case BackwardReceived:
					ESP_LOGI(" ", "BackwardReceived");
					speed_multiplier = -1;
					changeMotorSpeed(0);
					vTaskDelay(pdMS_TO_TICKS(50));
					changeMotorSpeed(-15);
					vTaskDelay(pdMS_TO_TICKS(50));
					changeMotorSpeed(0);
//					vTaskDelay(pdMS_TO_TICKS(50));
//					changeMotorSpeed(-15);
					vTaskDelay(pdMS_TO_TICKS(50));
					changeMotorSpeed(speed_multiplier * last_motor_speed); // to be replaced with notify task PID.
					break;
				case SpeedReceived:
					changeMotorSpeed(speed_multiplier * cmd.command_value);
					last_motor_speed = cmd.command_value;
					ESP_LOGI(" ", "SpeedReceived %d", speed_multiplier); // to be replaced with notify task PID.
					break;
				case SteerReceived:
					changeSTEER(cmd.command_value);
					//ESP_LOGI(" ", "SteerReceived");
					break;
				case AutonomousReceived:
					//ESP_LOGI(" ", "AutonomousReceived");
					break;
				}
			}
		} else
			vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void carControl_init() {
	carControlQueue = xQueueCreate(10,sizeof(char*));
	init_servo_pwm();
	init_motor_pwm();
	update_servo_pwm(1500); // ESC init
	update_motor_pwm(1500);
	//configureEncoderInterrupts();
	vTaskDelay(pdMS_TO_TICKS(500)); // wait for init to complete
	xTaskCreatePinnedToCore(carControl_Task, "carControl_task", 4096, NULL, 6,
	NULL, 0U);

}

