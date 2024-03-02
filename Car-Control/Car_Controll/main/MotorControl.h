#ifndef MOTORCONTROL
#define MOTORCONTROL

#include <stdio.h>
#include "driver/mcpwm_prelude.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/mcpwm.h"
//#include "esp_system.h"
//#include "driver/mcpwm_types_legacy.h"
#include "driver/mcpwm_types.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
#include "unity.h"
#include "soc/soc_caps.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/gpio.h"
#include <stdio.h>
#include "sdkconfig.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/semphr.h"
//#include "freertos/task.h"
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
//#include "driver/mcpwm_types_legacy.h"
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


typedef struct {
    mcpwm_timer_handle_t timer;
    mcpwm_cmpr_handle_t comparator_a;
    mcpwm_cmpr_handle_t comparator_b;
} mcpwm_handles_t;

mcpwm_handles_t init_mcpwm(uint32_t frequency, uint32_t dead_time, int gpio_a, int gpio_b);

typedef enum {
    NORMAL,
    FORCE_HIGH
} pin_action_t;

void control_mcpwm(mcpwm_handles_t handler, float duty_cycle, uint32_t frequency, mcpwm_action_t action, pin_action_t pinAction);

void motorControl(unsigned char dir, float speed, mcpwm_handles_t handler1, mcpwm_handles_t handler2);
enum motor_state {
    FORWARD,
    BACKWARD,
    STOP,
    BREAKING,
	NONE
};




void init_adc();

int read_analog_value();


#endif
