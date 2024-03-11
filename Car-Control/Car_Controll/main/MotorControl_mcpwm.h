#ifndef MOTORCONTROL_MCPWM
#define MOTORCONTROL_MCPWM

#include <stdio.h>
#include "driver/mcpwm_prelude.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm_types.h"
#include "unity.h"
#include "soc/soc_caps.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/gpio.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_rom_gpio.h"
#include "esp_intr_alloc.h"
#include "soc/gpio_periph.h"
#include "soc/mcpwm_periph.h"
#include "hal/mcpwm_hal.h"
#include "hal/gpio_hal.h"
#include "hal/mcpwm_ll.h"
#include "driver/gpio.h"
#include "esp_private/periph_ctrl.h"
#include "driver/adc.h"

#define Driver1High 23
#define Driver1Low 19
#define Driver2High 18
#define Driver2Low 13

#define FREQUENCY 20000 // 20khz
#define DEAD_TIME 2 // 2us
#define ALPHA 0.1  // Smoothing factor, between 0 and 1

typedef enum {
    MCPWM_START,
    MCPWM_STOP,
    MCPWM_UPDATE
} mcpwm_action_t;

enum motor_state {
    FORWARD,
    BACKWARD,
    STOP,
    BREAKING,
	NONE
};

typedef struct {
    mcpwm_timer_handle_t timer;
    mcpwm_cmpr_handle_t comparator_a;
    mcpwm_cmpr_handle_t comparator_b;
} mcpwm_handles_t;

typedef enum {
    NORMAL,
    FORCE_HIGH
} pin_action_t;

void control_mcpwm(mcpwm_handles_t handler, float duty_cycle, uint32_t frequency, mcpwm_action_t action, pin_action_t pinAction);
void motorControl_mcpwm(unsigned char dir, float speed, mcpwm_handles_t handler1, mcpwm_handles_t handler2);
mcpwm_handles_t init_mcpwm(uint32_t frequency, uint32_t dead_time, int gpio_a, int gpio_b);



//mcpwm_handles_t handler1; // MOSFET DRIVER 1
//mcpwm_handles_t handler2; // MOSFET DRIVER 2
//
//TaskHandle_t controlMotorHandle = NULL;
//TaskHandle_t controlSteerHandle = NULL;
//
//commandReceived_app mainCommand;
//
//void controlMotor_mcpwm(void *pvParameters) {
//	uint32_t notificationValue;
//	while (1) {
//		notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//		if (notificationValue == FORWARD) {
//			motorControl_mcpwm(FORWARD, 100 - SPEED, handler1, handler2);
//		} else if (notificationValue == BACKWARD) {
//			motorControl_mcpwm(BACKWARD, 100 - SPEED, handler1, handler2);
//		} else if (notificationValue == BREAKING) {
//			motorControl_mcpwm(BREAKING, 100, handler1, handler2);
//		} else if (notificationValue == STOP) {
//			motorControl_mcpwm(STOP, 100, handler1, handler2);
//		}
//	}
//}
//void init_adc();
//int read_analog_value();


#endif
