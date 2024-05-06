#include "UART_Jetson.h"
#include "MotorAndServoControl.h"

extern bool AutonomousMode;
QueueHandle_t uartJetsonQueue;

void JetsonUartConfig()
{
	const uart_port_t uart_num = UART_PORT;
	uart_config_t uart_config = {
	    .baud_rate = UART_BAUD,
	    .data_bits = UART_WORD_LENGTH_BITS,
	    .parity = UART_PARITY_DISABLE,
	    .stop_bits = UART_STOP_BITS_1,
	    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};
	// Configure UART parameters
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	// Install UART driver using an event queue
	ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUFFER_SIZE, UART_BUFFER_SIZE, UART_Queue_SIZE, &uartJetsonQueue, 0));
}

void uart_Jetson_Task (void *params)
{
	uart_event_t uartJetsonEvent;
	uint8_t data[128];
	int length = 0;
	while(1)
	{
		if(AutonomousMode == false)
		{
			vTaskDelay(pdMS_TO_TICKS(100));
			uart_flush(UART_PORT);
			xQueueReset(uartJetsonQueue);
		}
		else if (xQueueReceive(uartJetsonQueue, (void *)&uartJetsonEvent, (TickType_t)portMAX_DELAY))
		{
			switch(uartJetsonEvent.type)
			{
				case UART_DATA:
					ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_PORT, (size_t*)&length));
					uart_read_bytes(UART_PORT, data, length, portMAX_DELAY);
					data[length]='\0';
					ESP_LOGI("Data Jetson", "Data Received: %s", data);
					//parse data

					//filter incomming data: if the change is too small in a too short
					//amount of time then don't make a change in steer.


				break;

				case UART_BUFFER_FULL:
					ESP_LOGI("Event Uart ", "ring buffer full");
					// If buffer full happened, you should consider increasing your buffer size
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(UART_PORT);
					xQueueReset(uartJetsonQueue);
				break;

				case UART_FIFO_OVF:
					ESP_LOGI("Event Uart ", "ring buffer ovf");
					uart_flush_input(UART_PORT);
					xQueueReset(uartJetsonQueue);
				break;

				default:
				ESP_LOGI("Uart Event:", "event type: %d", uartJetsonEvent.type);
				break;
			}
		}
	}
}

void start_UartJetson_task()
{
	JetsonUartConfig();
	xTaskCreatePinnedToCore(uart_Jetson_Task, "uartJetson", 4096, NULL, 6, NULL,1U);
}

CarCommand parseJetsonData(const uint8_t* data)
{
	int currentSpeed = 0;
	int currentSteer = 0;
	static int lastSpeed = 0;
	static int lastSteer = 0;
	static TickType_t newTime = 0U;
	static TickType_t oldTime = 0U;
	CarCommand Jet;
	memset(&Jet, 0, sizeof(Jet));
	sscanf(data, "st=%d sp=%d", &currentSpeed, &currentSteer);

}
