#include "MotorAndServoControl.h"
/*Include Network.h to send back to app diagnostics from the car*/
#include "I2C_cameraSensor.h"
#include "Network.h"

mcpwm_timer_handle_t timer_servo;
mcpwm_oper_handle_t operator_servo;
mcpwm_cmpr_handle_t comparator_servo;
mcpwm_gen_handle_t generator_servo;

mcpwm_timer_handle_t timer_dc_motor;
mcpwm_oper_handle_t operator_dc_motor;
mcpwm_cmpr_handle_t comparator_dc_motor;
mcpwm_gen_handle_t generator_dc_motor;

//Queue set which adds all data from all queues.
QueueSetHandle_t QueueSetGeneralCommands;
extern QueueHandle_t autonomousModeControlPixyQueue;

// Initialize PWM for SERVO
void init_servo_pwm() {

#if (MOTOR_CONTROL == LEDC)
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
#elif (MOTOR_CONTROL == MCPWM)

	// Configure MCPWM timer
	mcpwm_timer_config_t config =
	{
		.group_id = 0,
		.intr_priority = 0,
		.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
		// period_ticks=Desired Period * Resolution
		// period_ticks= 20000 microseconds * 1 microsecond
		// 1MHZ => 1 tick = 1/1000000 = 1 microsecond each tick
		.resolution_hz = 1000000,
		// 20000 * 1 microsecond = 20000 microseconds => 0.02 seconds => 50 hz
		.period_ticks = 20000,
		.count_mode = MCPWM_TIMER_COUNT_MODE_UP,
		.flags.update_period_on_empty = true, // Update period when timer counts to zero
		.flags.update_period_on_sync = false, // Do not update period on sync signal
	};

	ESP_ERROR_CHECK(mcpwm_new_timer(&config, &timer_servo));

	// Configure MCPWM operator
	mcpwm_operator_config_t operator_config = {
		.group_id = 0,
		.intr_priority = 0,
		.flags.update_gen_action_on_tez = 1, // Update generator action when timer counts to zero
		.flags.update_gen_action_on_tep = 0, // Do not update generator action when timer counts to peak
		.flags.update_gen_action_on_sync = 0 // Do not update generator action on sync signal
	};

	ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &operator_servo));
	ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_servo, timer_servo));

	// Configure MCPWM comparator
	mcpwm_comparator_config_t comparator_config =
	{
			.intr_priority = 0,
			//This ensures that the compare value (duty cycle) is updated when the timer count reaches zero.
			.flags.update_cmp_on_tez = 1,
			.flags.update_cmp_on_tep = 0, // Do not update compare value when timer count equals peak
			.flags.update_cmp_on_sync = 0 // Do not update compare value on sync event
	};

	ESP_ERROR_CHECK(mcpwm_new_comparator(operator_servo, &comparator_config, &comparator_servo));

	// Configure MCPWM generator
	mcpwm_generator_config_t generator_config = {
	    .gen_gpio_num = LEDC_SERVO_GPIO, // Set the GPIO number used by the generator
	    .flags.invert_pwm = false, // Do not invert the PWM signal
	    .flags.io_loop_back = false, // Disable loop-back mode
	    .flags.io_od_mode = false, // Do not configure as open-drain output
	    .flags.pull_up = false, // Disable internal pull-up resistor
	    .flags.pull_down = false, // Disable internal pull-down resistor
	};

	ESP_ERROR_CHECK(mcpwm_new_generator(operator_servo, &generator_config, &generator_servo));

	// Set the generator actions
	 // go high on counter empty
	ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_servo,
															  MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
	// go low on compare threshold
	ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_servo,
																MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_servo, MCPWM_GEN_ACTION_LOW)));

	// Set the duty cycle for the servo
	ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_servo, 1500)); // 1.5ms pulse width for neutral position

	// Start the MCPWM timer
	ESP_ERROR_CHECK(mcpwm_timer_enable(timer_servo));
	ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_servo, MCPWM_TIMER_START_NO_STOP));

#endif
}

// Update the PWM duty cycle for SERVO
void update_servo_pwm(int pulse_width_us) {
#if (MOTOR_CONTROL == LEDC)
    uint32_t duty = (pulse_width_us * ((1 << LEDC_SERVO_DUTY_RESOLUTION) - 1)) / 20000;
    ledc_set_duty(LEDC_Motor_And_Servo_MODE, LEDC_SERVO_CH0_CHANNEL, duty);
    ledc_update_duty(LEDC_Motor_And_Servo_MODE, LEDC_SERVO_CH0_CHANNEL);
#elif (MOTOR_CONTROL == MCPWM)
    esp_err_t err = mcpwm_comparator_set_compare_value(comparator_servo, pulse_width_us);
#endif
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
#if (MOTOR_CONTROL == LEDC)
	// Timer configuration
	ledc_timer_config_t ledc_timer =
	{
		.speed_mode = LEDC_Motor_And_Servo_MODE,
		.duty_resolution = LEDC_MOTOR_DUTY_RESOLUTION,
		.timer_num = LEDC_MOTOR_TIMER,  				// Different timer for motor
		.freq_hz = LEDC_Motor_And_Servo_FREQ,
	};
	ledc_timer_config(&ledc_timer);

	// Channel configuration, similar to steering but different channel and GPIO
	ledc_channel_config_t ledc_channel =
	{
		.channel = LEDC_MOTOR_CH1_CHANNEL, 		 // Different channel for motor
		.duty = 0,
		.gpio_num = LEDC_MOTOR_GPIO,  			 // Different GPIO for motor
		.speed_mode = LEDC_Motor_And_Servo_MODE,
		.timer_sel = LEDC_MOTOR_TIMER,    		 // Different timer for motor
	};
	ledc_channel_config(&ledc_channel);
#elif (MOTOR_CONTROL == MCPWM)

	// Configure MCPWM timer
	mcpwm_timer_config_t config =
	{
		.group_id = 1,
		//.intr_priority = 0,
		.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
		// period_ticks=Desired Period * Resolution
		// period_ticks= 20000 microseconds * 1 microsecond
		.resolution_hz = 1000000, // 1MHZ => 1 tick = 1/1000000 = 1 microsecond each tick
		.period_ticks = 20000,    // 20000 * 1 microsecond = 20000 microseconds => 0.02 seconds => 50 hz
		.count_mode = MCPWM_TIMER_COUNT_MODE_UP,
	};

	ESP_ERROR_CHECK(mcpwm_new_timer(&config, &timer_dc_motor));

	// Configure MCPWM operator
	mcpwm_operator_config_t operator_config = {
		.group_id = 1,
		.flags.update_gen_action_on_tez = 1, // Update generator action when timer counts to zero
	};

	ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &operator_dc_motor));
	ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_dc_motor, timer_dc_motor));

	// Configure MCPWM comparator
	mcpwm_comparator_config_t comparator_config =
	{
			.flags.update_cmp_on_tez = 1,
	};

	ESP_ERROR_CHECK(mcpwm_new_comparator(operator_dc_motor, &comparator_config, &comparator_dc_motor));

	// Configure MCPWM generator
	mcpwm_generator_config_t generator_config = {
		.gen_gpio_num = LEDC_MOTOR_GPIO, // Set the GPIO number used by the generator
	};

	ESP_ERROR_CHECK(mcpwm_new_generator(operator_dc_motor, &generator_config, &generator_dc_motor));

	// Set the generator actions
	 // go high on counter empty
	ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_dc_motor,
															  MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
	// go low on compare threshold
	ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_dc_motor,
																MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_dc_motor, MCPWM_GEN_ACTION_LOW)));

	// Set the duty cycle for the servo
	ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_dc_motor, 1500)); // 1.5ms pulse width for neutral position

	// Start the MCPWM timer
	ESP_ERROR_CHECK(mcpwm_timer_enable(timer_dc_motor));
	ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_dc_motor, MCPWM_TIMER_START_NO_STOP));


#endif
}

// Update the PWM for Motor, similar to update_servo_pwm function
void update_motor_pwm(unsigned int pulse_width_us) {
#if (MOTOR_CONTROL == LEDC)
	uint32_t duty = (pulse_width_us * ((1 << LEDC_MOTOR_DUTY_RESOLUTION) - 1))
			/ 20000;
	ledc_set_duty(LEDC_Motor_And_Servo_MODE, LEDC_MOTOR_CH1_CHANNEL, duty);
	ledc_update_duty(LEDC_Motor_And_Servo_MODE, LEDC_MOTOR_CH1_CHANNEL);
#elif (MOTOR_CONTROL == MCPWM)
	ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_dc_motor, pulse_width_us));
#endif
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

bool IRAM_ATTR example_pcnt_on_reach(pcnt_unit_handle_t unit,
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

	pcnt_chan_config_t chan_a_config = {
			.edge_gpio_num = encoderGPIO_B,
			.level_gpio_num = encoderGPIO_A,
	};
	pcnt_channel_handle_t pcnt_chan_a = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

	pcnt_chan_config_t chan_b_config = {
			.edge_gpio_num = encoderGPIO_A,
			.level_gpio_num = encoderGPIO_B,
	};
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

	ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, pulse_encoderQueue));
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

void carControl_Backward_init(int break_value)
{
	changeMotorSpeed(0);
	vTaskDelay(pdMS_TO_TICKS(50));
	changeMotorSpeed(break_value);
	vTaskDelay(pdMS_TO_TICKS(50));
	changeMotorSpeed(0);
	vTaskDelay(pdMS_TO_TICKS(50));
	changeMotorSpeed(break_value);
	vTaskDelay(pdMS_TO_TICKS(50));
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
	//ESP_LOGI(" ", "ESC Calibration over.");
	calib_motor_done = true;
}

static void mock_forward_and_backward_test_5_seconds()
{

	/* MOVING FORWARD */
	update_motor_pwm(1600);
	vTaskDelay(pdMS_TO_TICKS(5000));

	/*BACKWARD MODE INIT*/
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(1000));
	update_motor_pwm(1400);
	vTaskDelay(pdMS_TO_TICKS(1000));
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(1000));

	/* MOVING BACKWARD */
	update_motor_pwm(1400);
	vTaskDelay(pdMS_TO_TICKS(5000));

	/*STOP*/
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(5000));
}

QueueHandle_t diagnosticModeControlQueue = NULL;
QueueHandle_t autonomousModeControlUartQueue = NULL;
QueueHandle_t speed_commandQueue = NULL;
QueueHandle_t steer_commandQueue = NULL;
QueueHandle_t PID_commandQueue = NULL;
QueueSetHandle_t QueueSetAutonomousOrDiagnostic = NULL;

/* This variable allows the uart_task to fill Autonomous Queue for controlling the car. */
bool AutonomousMode = false;
void carControl_Task(void *pvParameters) {

#if (MOTOR_MOCK_TEST == ON)
	while(1)
	{
		mock_forward_and_backward_test_5_seconds();
	}
#elif (MOTOR_MOCK_TEST == OFF)
	CarCommand cmd = { StopReceived, 0, false, 0.0f, 0.0f, 0.0f };
	int last_motor_speed = 0;
	int speed_multiplier = 0;
	int speed = 0;
	char *command;
	QueueSetMemberHandle_t xActivatedModeAorM_Member;

	while (1) {
		if (diagnosticModeControlQueue != NULL && (autonomousModeControlUartQueue != NULL || autonomousModeControlPixyQueue != NULL)) {
			xActivatedModeAorM_Member = xQueueSelectFromSet(QueueSetAutonomousOrDiagnostic, portMAX_DELAY);

			if(xActivatedModeAorM_Member == diagnosticModeControlQueue)
			{
				AutonomousMode = false;
				xQueueReceive(xActivatedModeAorM_Member, &command, 0);
				cmd = parseCommand(command);
			}
#if (PIXY_DETECTION == OFF)
			else if(xActivatedModeAorM_Member == autonomousModeControlUartQueue)
			{
				xQueueReceive(xActivatedModeAorM_Member, &command, 0);

				if(strncmp(command,"sp=",3)==0)
				{
					sscanf(command, "sp=%d", &cmd.command_value);
					cmd.command = SpeedReceived;
				}
				else if(strncmp(command,"st=",3)==0)
				{
					sscanf(command, "st=%d", &cmd.command_value);
					cmd.command = SteerReceived;
				}
			}
#elif (PIXY_DETECTION == ON)
			else if(xActivatedModeAorM_Member == autonomousModeControlPixyQueue)
			{
				static uint8_t nrCmd = 0U;
				nrCmd++;
				xQueueReceive(xActivatedModeAorM_Member, &cmd.command_value, 0);
				if(nrCmd == 1U)
				{
					cmd.command = SpeedReceived;
					ESP_LOGI("PIXY","Speed: %d", cmd.command_value);
				}
				else if(nrCmd == 2U)
				{
					cmd.command = SteerReceived;
					ESP_LOGI("PIXY","Steer: %d", cmd.command_value);
					nrCmd = 0U;
				}


			}
#endif


				switch (cmd.command) {
				case StopReceived:
					speed = 0;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					speed_multiplier = 0;
					break;
				case ForwardReceived:
					speed_multiplier = 1;
					speed = speed_multiplier * last_motor_speed;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					HLD_SendMessage("OKFWD!");
					break;
				case BackwardReceived:
					speed_multiplier = -1;
					speed = speed_multiplier * last_motor_speed;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					HLD_SendMessage("OKBWD!");
					break;
				case SpeedReceived:
					if(AutonomousMode == false)
						speed = speed_multiplier * cmd.command_value;
					else
						speed = cmd.command_value;

					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					last_motor_speed = cmd.command_value;
					HLD_SendMessage("OKSPEED!");
					break;
				case SteerReceived:
					xQueueSend(steer_commandQueue,&cmd.command_value,portMAX_DELAY);
					HLD_SendMessage("OKSTEER!");
					break;
				case AutonomousReceived:
					AutonomousMode = true;
					ESP_LOGI("Auton", "");
					break;
				case PID_Changed:
					xQueueSend(PID_commandQueue,&cmd,portMAX_DELAY);
					HLD_SendMessage("OKPID!");
					break;
				}

		} else
			vTaskDelay(pdMS_TO_TICKS(250));
	}
#endif
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
	vTaskDelay(pdMS_TO_TICKS(2000));
	update_motor_pwm(1500);
	//carControl_calibrate_motor();
	//while(calib_motor_done!=true) vTaskDelay(pdMS_TO_TICKS(50));
	vTaskDelay(pdMS_TO_TICKS(500)); // wait for init to complete
	xTaskCreatePinnedToCore(carControl_Task, "carControl_task", 4096, NULL, 6,
	NULL, 0U);
	xTaskCreatePinnedToCore(steer_task, "steer_task", 4096, NULL, 5,NULL, 1U);

}

