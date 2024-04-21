/*Brief for on which cores are the tasks runing
	PIDTask: 		 	Core 1, Prio 10
	CarControl_Task: 	Core 0, Prio 6
	steer_task: 	 	Core 1, Prio 6
	udp_server_task: 	Core 0, Prio 5
	i2c_task: 		 	Core 1, Prio 7
*/

/*Brief for the main function
	1. Create the necessary queues for the tasks.
	2. Create a set which holds data from all necessary queues.
	3. Start the network tasks.
	4. Initialize the car control.
	5. Configure the encoder interrupts.
	6. Create the PID task.
*/

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "MotorAndServoControl.h"
#include "PID.h"
#include "Network.h"
#include "I2C_devices.h"

extern bool allowed_to_send;
extern char rx_buffer[128];
//Autonomous + diagnostic modes
TaskHandle_t myTaskHandle_Autonomous = NULL;
TaskHandle_t myTaskHandle_diagnostic = NULL;

extern QueueHandle_t carControlQueue; // used for general incoming commands from the phone app
extern QueueHandle_t pulse_encoderQueue; // holds the trasnformed time pulse differences incomming from encoder.
extern QueueHandle_t speed_commandQueue;
extern QueueHandle_t steer_commandQueue;
extern QueueHandle_t PID_commandQueue;
extern QueueSetHandle_t QueueSet;
//#define ESP_LOGI(a,b) printf(b);
void app_main(void) {
	steer_commandQueue = xQueueCreate(10,QUEUE_SIZE_DATATYPE_ENCODER_PULSE);
	carControlQueue = xQueueCreate(QUEUE_SIZE_CAR_COMMANDS, QUEUE_SIZE_DATATYPE_CAR_COMMANDS);
	pulse_encoderQueue = xQueueCreate(QUEUE_SIZE_ENCODER_PULSE, QUEUE_SIZE_DATATYPE_ENCODER_PULSE);
	speed_commandQueue = xQueueCreate(QUEUE_SIZE_SPEED, QUEUE_SIZE_DATATYPE_SPEED);
	PID_commandQueue = xQueueCreate(5, sizeof(CarCommand));

	/*Create a set which holds data from all necessary queues.*/
	QueueSet = xQueueCreateSet(QUEUE_SIZE_CAR_COMMANDS + QUEUE_SIZE_ENCODER_PULSE);
	configASSERT(speed_commandQueue);
	configASSERT(pulse_encoderQueue);
	configASSERT(speed_commandQueue);
	configASSERT(PID_commandQueue);
	configASSERT(steer_commandQueue);
	configASSERT(QueueSet);
	xQueueAddToSet(speed_commandQueue, QueueSet);
	xQueueAddToSet(pulse_encoderQueue, QueueSet);
	xQueueAddToSet(PID_commandQueue, QueueSet);
	start_network_readBuffer_tasks();
	carControl_init();
	while(allowed_to_send == false) vTaskDelay(pdMS_TO_TICKS(50));
	start_I2C_devices_task();
	configureEncoderInterrupts();
	xTaskCreatePinnedToCore(PIDTask, "PIDTask", 4096, NULL, 10, NULL, 1U);

}
