#include "MotorControl_mcpwm.h"

mcpwm_handles_t init_mcpwm(uint32_t frequency, uint32_t dead_time, int gpio_a, int gpio_b) {
    uint32_t timer_resolution = 10000000;  // 10MHz
    uint32_t period = timer_resolution / frequency;
    uint32_t dead_time_ticks = (timer_resolution * dead_time) / 1000000;  // convert dead time from us to ticks

    // Timer Configuration
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .resolution_hz = timer_resolution,
        .period_ticks = period,
    };
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_new_timer(&timer_config, &timer);

    // Operator Configuration
    mcpwm_operator_config_t operator_config = {
        .group_id = 0,
    };
    mcpwm_oper_handle_t oper = NULL;
    mcpwm_new_operator(&operator_config, &oper);
    mcpwm_operator_connect_timer(oper, timer);
    mcpwm_timer_enable(timer);

    // Comparator Configuration
    mcpwm_cmpr_handle_t comparator_a = NULL;
    mcpwm_cmpr_handle_t comparator_b = NULL;
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    mcpwm_new_comparator(oper, &comparator_config, &comparator_a);
    mcpwm_new_comparator(oper, &comparator_config, &comparator_b);

    // Generator Configuration
    mcpwm_gen_handle_t generator_a = NULL;
    mcpwm_gen_handle_t generator_b = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = gpio_a,
    };
    mcpwm_new_generator(oper, &generator_config, &generator_a);
    generator_config.gen_gpio_num = gpio_b;
    mcpwm_new_generator(oper, &generator_config, &generator_b);

    // Dead Time Configuration
    mcpwm_dead_time_config_t dead_time_config = {
        .posedge_delay_ticks = dead_time_ticks,
        .negedge_delay_ticks = 0
    };
    mcpwm_generator_set_dead_time(generator_a, generator_a, &dead_time_config);
    dead_time_config.posedge_delay_ticks = 0;
    dead_time_config.negedge_delay_ticks = dead_time_ticks;
    dead_time_config.flags.invert_output = true;
    mcpwm_generator_set_dead_time(generator_a, generator_b, &dead_time_config);

    // Action on Timer and Compare Events
    mcpwm_generator_set_action_on_timer_event(generator_a,
                MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_FULL, MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_action_on_compare_event(generator_a,
                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_a, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_timer_event(generator_b,
                MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_FULL, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(generator_b,
                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_b, MCPWM_GEN_ACTION_LOW));

    // Start the MCPWM
    mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);

    mcpwm_handles_t handles = { timer, comparator_a, comparator_b };
    return handles;
}


void control_mcpwm(mcpwm_handles_t handler, float duty_cycle, uint32_t frequency, mcpwm_action_t action, pin_action_t pinAction) {
    uint32_t timer_resolution = 10000000;  // 10MHz
    uint32_t period = timer_resolution / frequency;
    uint32_t cmp_value = (uint32_t)(period * duty_cycle / 100);  // compute compare value from duty cycle

    // Update the comparator values


    switch(action) {
        case MCPWM_START:
            mcpwm_comparator_set_compare_value(handler.comparator_a, cmp_value);
            mcpwm_comparator_set_compare_value(handler.comparator_b, cmp_value);
            mcpwm_timer_start_stop(handler.timer, MCPWM_TIMER_START_NO_STOP);
            break;
        case MCPWM_STOP:
            mcpwm_comparator_set_compare_value(handler.comparator_a, cmp_value);
            mcpwm_comparator_set_compare_value(handler.comparator_b, cmp_value);
            mcpwm_timer_start_stop(handler.timer, MCPWM_TIMER_STOP_FULL);
            break;
        default:
            // Handle invalid action here if needed
            break;
    }

    // Handle pin action
    if (pinAction == FORCE_HIGH) {
        // Forcing the low driver pin to HIGH by setting duty cycle to 100%
        mcpwm_comparator_set_compare_value(handler.comparator_b, cmp_value); // 100% duty cycle
    }
}



void motorControl_mcpwm(unsigned char dir, float speed, mcpwm_handles_t handler1, mcpwm_handles_t handler2) {
    switch (dir) {
        case FORWARD:
            // Stop MCPWM for backward direction
        	control_mcpwm(handler1, speed, FREQUENCY, MCPWM_STOP, NORMAL);
            control_mcpwm(handler2, speed, FREQUENCY, MCPWM_STOP, NORMAL);
            vTaskDelay(pdMS_TO_TICKS(50));
            // Optional: Force the LOW driver pin to HIGH
            control_mcpwm(handler2, 100, FREQUENCY, MCPWM_START, FORCE_HIGH);

            // Start MCPWM for forward direction
            control_mcpwm(handler1, speed, FREQUENCY, MCPWM_START, NORMAL);
            break;

        case BACKWARD:
            // Stop MCPWM for forward direction
        	control_mcpwm(handler1, speed, FREQUENCY, MCPWM_STOP, NORMAL);
        	control_mcpwm(handler2, speed, FREQUENCY, MCPWM_STOP, NORMAL);
        	vTaskDelay(pdMS_TO_TICKS(50));
            // Optional: Force the LOW driver pin to HIGH
            control_mcpwm(handler1, 100, FREQUENCY, MCPWM_START, FORCE_HIGH);

            // Start MCPWM for backward direction
            control_mcpwm(handler2, speed, FREQUENCY, MCPWM_START, NORMAL);
            break;
	       case STOP:

	    	   control_mcpwm(handler1, speed, FREQUENCY, MCPWM_STOP, NORMAL);
	    	   control_mcpwm(handler2, speed, FREQUENCY, MCPWM_STOP, NORMAL);
	    	   vTaskDelay(pdMS_TO_TICKS(50));
	    	   control_mcpwm(handler1, speed, FREQUENCY, MCPWM_STOP, FORCE_HIGH);
	    	   control_mcpwm(handler2, speed, FREQUENCY, MCPWM_STOP, FORCE_HIGH);

	           break;
	       case BREAKING:
	    	   // Delay for 20 ms
	    	   control_mcpwm(handler1, speed, FREQUENCY, MCPWM_STOP, NORMAL);
	    	   control_mcpwm(handler2, speed, FREQUENCY, MCPWM_STOP, NORMAL);
	    	   vTaskDelay(pdMS_TO_TICKS(50));
	    	   control_mcpwm(handler1, 100, FREQUENCY, MCPWM_START, FORCE_HIGH);
	    	   control_mcpwm(handler2, 100, FREQUENCY, MCPWM_START, FORCE_HIGH);

	           break;
	       default:

	           break;
	   }
}


//void init_adc() {
//    adc1_config_width(ADC_WIDTH_BIT_12);
//    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_12);  // GPIO33 is ADC1_CHANNEL_5
//}
//
//int read_analog_value() {
//	static int ema = 0;
//    int val = adc1_get_raw(ADC1_CHANNEL_5);
//    ema = (int)((1.0 - ALPHA) * ema + ALPHA * val);
//
//    // Map 0-4095 to 0-100
//    int speed = (ema * 100) / 4095;
//
//    vTaskDelay(pdMS_TO_TICKS(1));
//    return speed;
//}

