/*
 * DcMotor.c
 *
 *  Created on: 18 iun. 2024
 *      Author: racov
 */
#include "DcMotor.h"

mcpwm_timer_handle_t timer_dc_motor;
mcpwm_oper_handle_t operator_dc_motor;
mcpwm_cmpr_handle_t comparator_dc_motor;
mcpwm_gen_handle_t generator_dc_motor;


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
	//ESP_LOGI("pulse_width_us", "us = %d", pulse_width_us);
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
