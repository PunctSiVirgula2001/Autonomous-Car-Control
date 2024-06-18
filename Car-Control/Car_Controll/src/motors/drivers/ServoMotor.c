/*
 * ServoMotor.c
 *
 *  Created on: 18 iun. 2024
 *      Author: racov
 */

#include "ServoMotor.h"

mcpwm_timer_handle_t timer_servo;
mcpwm_oper_handle_t operator_servo;
mcpwm_cmpr_handle_t comparator_servo;
mcpwm_gen_handle_t generator_servo;

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

