/*
 * encoder.c
 *
 *  Created on: 18 iun. 2024
 *      Author: racov
 */

#include "encoder.h"

/* Encoder Configuration */
QueueHandle_t pulse_encoderQueue;
pcnt_unit_handle_t encoderPcntUnit = NULL;

/* Interrupt Service Routine for the Pulse Counter */
bool IRAM_ATTR encoder_pcnt_isr(pcnt_unit_handle_t unit,
    const pcnt_watch_event_data_t *edata, void *user_context) {

    BaseType_t higherPriorityTaskWoken;
    QueueHandle_t queue = (QueueHandle_t) user_context;

    // Send the event data to the queue from the ISR
    xQueueSendFromISR(queue, &(edata->watch_point_value), &higherPriorityTaskWoken);
    // Clear the pulse count to prepare for the next count
    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoderPcntUnit));

    return (higherPriorityTaskWoken == pdTRUE);
}

/* Function to configure encoder and its interrupts */
void initializeEncoder() {
    // Configure the PCNT unit with high and low watch points
    pcnt_unit_config_t encoderUnitConfig = {
        .high_limit = PULSE_COUNT_HIGH_LIMIT,
        .low_limit = PULSE_COUNT_LOW_LIMIT,
    };

    ESP_ERROR_CHECK(pcnt_new_unit(&encoderUnitConfig, &encoderPcntUnit));

    // Set up glitch filter to debounce the encoder signals
    pcnt_glitch_filter_config_t glitchFilterConfig = {
        .max_glitch_ns = 1000,  // Debounce filter setting
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(encoderPcntUnit, &glitchFilterConfig));

    // Add watch points to the PCNT unit for detecting pulse limits
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(encoderPcntUnit, PULSE_COUNT_HIGH_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(encoderPcntUnit, PULSE_COUNT_LOW_LIMIT));

    // Configure the channels for the encoder signals
    pcnt_chan_config_t channelAConfig = {
        .edge_gpio_num = encoderGPIO_B,
        .level_gpio_num = encoderGPIO_A,
    };
    pcnt_channel_handle_t pcntChannelA = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(encoderPcntUnit, &channelAConfig, &pcntChannelA));

    pcnt_chan_config_t channelBConfig = {
        .edge_gpio_num = encoderGPIO_A,
        .level_gpio_num = encoderGPIO_B,
    };
    pcnt_channel_handle_t pcntChannelB = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(encoderPcntUnit, &channelBConfig, &pcntChannelB));

    // Set edge and level actions for the channels to handle pulse counting
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcntChannelA,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,   // B on positive action
        PCNT_CHANNEL_EDGE_ACTION_INCREASE)); // B on negative action
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcntChannelA,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,		 // A high level
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE)); // A low level
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcntChannelB,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,   // A on positive action
        PCNT_CHANNEL_EDGE_ACTION_DECREASE)); // A on negative action
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcntChannelB,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,		 // B high level
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE)); // B low level

    // Register the ISR callback for the PCNT unit
    pcnt_event_callbacks_t pcntCallbacks = {
        .on_reach = encoder_pcnt_isr,
    };

    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(encoderPcntUnit, &pcntCallbacks, pulse_encoderQueue));
    // Enable the PCNT unit
    ESP_ERROR_CHECK(pcnt_unit_enable(encoderPcntUnit));
    // Clear the count before starting
    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoderPcntUnit));
    // Start the pulse counter
    ESP_ERROR_CHECK(pcnt_unit_start(encoderPcntUnit));
}


