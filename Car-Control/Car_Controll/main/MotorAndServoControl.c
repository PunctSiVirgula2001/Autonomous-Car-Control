#include "MotorAndServoControl.h"

// Initialize PWM for SERVO
void init_servo_pwm() {
    // Prepare and set configuration of timers
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_Motor_And_Servo_MODE,
        .duty_resolution = LEDC_SERVO_DUTY_RESOLUTION,
        .timer_num = LEDC_SERVO_TIMER,
        .freq_hz = LEDC_Motor_And_Servo_FREQ,
    };
    ledc_timer_config(&ledc_timer);

    // Prepare individual configuration
    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_SERVO_CH0_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_SERVO_GPIO,
        .speed_mode = LEDC_Motor_And_Servo_MODE,
        .timer_sel  = LEDC_SERVO_TIMER,
    };
    ledc_channel_config(&ledc_channel);
}

// Update the PWM duty cycle for SERVO
void update_servo_pwm(int pulse_width_us) {
    uint32_t duty = (pulse_width_us * ((1 << LEDC_SERVO_DUTY_RESOLUTION) - 1)) / 20000;
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
    int scaled_value = 950 + (value * 10);
    update_servo_pwm(scaled_value);
}


// Initialize PWM for Motor
void init_motor_pwm() {
    // Timer configuration, same as before
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_Motor_And_Servo_MODE,
        .duty_resolution = LEDC_MOTOR_DUTY_RESOLUTION,
        .timer_num = LEDC_MOTOR_TIMER,  // Different timer for motor
        .freq_hz = LEDC_Motor_And_Servo_FREQ,
    };
    ledc_timer_config(&ledc_timer);

    // Channel configuration, similar to steering but different channel and GPIO
    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_MOTOR_CH1_CHANNEL,  // Different channel for motor
        .duty       = 0,
        .gpio_num   = LEDC_MOTOR_GPIO,     // Different GPIO for motor
        .speed_mode = LEDC_Motor_And_Servo_MODE,
        .timer_sel  = LEDC_MOTOR_TIMER,    // Different timer for motor
    };
    ledc_channel_config(&ledc_channel);
}

// Update the PWM for Motor, similar to update_servo_pwm function
void update_motor_pwm(unsigned int pulse_width_us) {
    uint32_t duty = (pulse_width_us * ((1 << LEDC_MOTOR_DUTY_RESOLUTION) - 1)) / 20000;
    ledc_set_duty(LEDC_Motor_And_Servo_MODE, LEDC_MOTOR_CH1_CHANNEL, duty);
    ledc_update_duty(LEDC_Motor_And_Servo_MODE, LEDC_MOTOR_CH1_CHANNEL);
}

// Change Motor Speed, similar to changeSTEER function

static int reverse_mode_activated = 0;  // Flag to track reverse mode activation
static int last_value = 100;  // Variable to keep track of the last value

void changeMotorSpeed(int value) {
    // Check if the value is within the expected range
    if (value < 0) {
        value = 0;
    } else if (value > 200) {
        value = 200;
    }

    int pulse_width_us;

    if (value >= 100) { // FORWARD
        // Forward mode: Scale between 1545 and 1700
        pulse_width_us = 1545 + ((value - 100) * (1700 - 1545) / (200 - 100));
    } else { 			// BACKWARD
        if (reverse_mode_activated == 0) {// Activate reverse mode sequence only once
            update_motor_pwm(1500);
            vTaskDelay(pdMS_TO_TICKS(20));

            update_motor_pwm(1445);
            vTaskDelay(pdMS_TO_TICKS(20));

            update_motor_pwm(1500);
            vTaskDelay(pdMS_TO_TICKS(20));

            reverse_mode_activated = 1;  // Set the flag
        }
        // Reverse mode: Scale between 1455 and 1240
        pulse_width_us = 1455 - ((100 - value) * (1455 - 1240) / (100 - 0));
    }

    update_motor_pwm(pulse_width_us);
    last_value = value;  // Update the last value for the next function call
}




