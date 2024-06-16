/*Brief for on which cores are the tasks runing
	PIDTask: 		 	Core 1, Prio 7
	CarControl_Task: 	Core 0, Prio 6
	steer_task: 	 	Core 1, Prio 6
	udp_server_task: 	Core 0, Prio 5
	i2c_task: 		 	Core 0, Prio 7
	uart_Jetson_Task	Core 1, Prio 6
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
#include "../include/I2C/I2C_sensorControl.h"
#include "UART_Jetson.h"

extern bool allowed_to_send;
extern char rx_buffer[128];

/* Used for directly controlling the car from the app. */
extern QueueHandle_t diagnosticModeControlQueue;
/* Used for the car for driving itself based on the information comming from either PIXY or JETSON. */
#if (PIXY_DETECTION == OFF)
extern QueueHandle_t autonomousModeControlUartQueue;
#elif (PIXY_DETECTION == ON)
extern QueueHandle_t autonomousModeControlPixyQueue;
#endif
/* Set which holds both Autonomous mode and Diagnostic mode queus.*/
extern QueueSetHandle_t QueueSetAutonomousOrDiagnostic;


extern QueueHandle_t pulse_encoderQueue; // holds the trasnformed time pulse differences incomming from encoder.
extern QueueHandle_t speed_commandQueue;
extern QueueHandle_t steer_commandQueue;
extern QueueHandle_t PID_commandQueue;
extern QueueHandle_t I2C_commandQueue;

extern QueueSetHandle_t QueueSetGeneralCommands;

extern bool I2C_sensors_initiated;
//#define ESP_LOGI(a,b) printf(b);

void app_main(void) {


	steer_commandQueue = xQueueCreate(20,QUEUE_SIZE_DATATYPE_ENCODER_PULSE);
	diagnosticModeControlQueue = xQueueCreate(QUEUE_SIZE_CAR_COMMANDS, QUEUE_SIZE_DATATYPE_CAR_COMMANDS);
#if (PIXY_DETECTION == OFF)
	autonomousModeControlUartQueue = xQueueCreate(QUEUE_SIZE_CAR_COMMANDS, QUEUE_SIZE_DATATYPE_CAR_COMMANDS);
#elif (PIXY_DETECTION == ON)
	autonomousModeControlPixyQueue = xQueueCreate(QUEUE_SIZE_PIXY, QUEUE_SIZE_DATATYPE_PIXY);
#endif
	pulse_encoderQueue = xQueueCreate(QUEUE_SIZE_ENCODER_PULSE, QUEUE_SIZE_DATATYPE_ENCODER_PULSE);
	speed_commandQueue = xQueueCreate(QUEUE_SIZE_SPEED, QUEUE_SIZE_DATATYPE_SPEED);
	PID_commandQueue = xQueueCreate(5, sizeof(CarCommand));
	I2C_commandQueue = xQueueCreate(20, I2C_COMMAND_SIZE);

	/* Create a set which would hold both Autonomous mode and Diagnostic mode queues */
	QueueSetAutonomousOrDiagnostic = xQueueCreateSet(QUEUE_SIZE_CAR_COMMANDS);
	xQueueAddToSet(diagnosticModeControlQueue, QueueSetAutonomousOrDiagnostic);
#if (PIXY_DETECTION == OFF)
	xQueueAddToSet(autonomousModeControlUartQueue, QueueSetAutonomousOrDiagnostic);
#elif (PIXY_DETECTION == ON)
	xQueueAddToSet(autonomousModeControlPixyQueue, QueueSetAutonomousOrDiagnostic);
#endif

	/* Create a set which holds data from all necessary queues. */
	QueueSetGeneralCommands = xQueueCreateSet(QUEUE_SIZE_CAR_COMMANDS + QUEUE_SIZE_ENCODER_PULSE + QUEUE_SIZE_I2C);
	configASSERT(speed_commandQueue);
	configASSERT(pulse_encoderQueue);
	configASSERT(speed_commandQueue);
	configASSERT(PID_commandQueue);
	configASSERT(steer_commandQueue);
	configASSERT(QueueSetGeneralCommands);
	xQueueAddToSet(speed_commandQueue, QueueSetGeneralCommands);
	xQueueAddToSet(pulse_encoderQueue, QueueSetGeneralCommands);
	xQueueAddToSet(PID_commandQueue, QueueSetGeneralCommands);
	xQueueAddToSet(I2C_commandQueue, QueueSetGeneralCommands);
	carControl_init();
#if (MOTOR_MOCK_TEST == OFF)
	start_I2C_devices_task();
	while(I2C_sensors_initiated == false) vTaskDelay(pdMS_TO_TICKS(50));
	start_network_task();
	while(allowed_to_send == false) vTaskDelay(pdMS_TO_TICKS(50));
	configureEncoderInterrupts();
	start_PID_task();
#if (PIXY_DETECTION == OFF)
	start_UartJetson_task();
#endif

#endif
}
