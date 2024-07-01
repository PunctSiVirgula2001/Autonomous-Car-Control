/*Brief for on which cores are the tasks running
	PIDTask: 		 	Core 1, Prio 7
	UartJetsonTask		Core 1, Prio 6
	SteerTask: 	 		Core 1, Prio 6
	UdpAccessPointTask: Core 0, Prio 5
	I2cTask: 		 	Core 0, Prio 7
    MotorAndSteerControlTask: 	Core 0, Prio 6

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

extern QueueSetHandle_t QueueSetPIDNecessaryCommands;

extern bool I2C_sensors_initiated;

void app_main(void) {

	/* Crearea cozilor specifice fiecarui tip de mesaj neceasr controlului.*/
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

	/*Crearea setului de cozi responsabil pentru modurile : Autonomous + Diagnostic*/
	QueueSetAutonomousOrDiagnostic = xQueueCreateSet(QUEUE_SIZE_CAR_COMMANDS);
	xQueueAddToSet(diagnosticModeControlQueue, QueueSetAutonomousOrDiagnostic);
#if (PIXY_DETECTION == OFF)
	xQueueAddToSet(autonomousModeControlUartQueue, QueueSetAutonomousOrDiagnostic);
#elif (PIXY_DETECTION == ON)
	xQueueAddToSet(autonomousModeControlPixyQueue, QueueSetAutonomousOrDiagnostic);
#endif

	/* Crearea setului de cozi pentru task-ul de PID. */
	QueueSetPIDNecessaryCommands = xQueueCreateSet(QUEUE_SIZE_CAR_COMMANDS + QUEUE_SIZE_ENCODER_PULSE + QUEUE_SIZE_I2C);
	xQueueAddToSet(speed_commandQueue, QueueSetPIDNecessaryCommands);
	xQueueAddToSet(pulse_encoderQueue, QueueSetPIDNecessaryCommands);
	xQueueAddToSet(PID_commandQueue, QueueSetPIDNecessaryCommands);
	xQueueAddToSet(I2C_commandQueue, QueueSetPIDNecessaryCommands);

	/* Start application tasks */

	/*Initializare PWM motor dc + servo si pornirea
	 * task-ului de control al directiei + parser-ul
	 * de comenzi de la user
	 * */
	carControl_init_and_start_CarControl_task();

	/*Initializare encoder.*/
	initializeEncoder();
#if (MOTOR_MOCK_TEST == OFF)
	/*Pornire task-ului care citeste datele de la senzorii i2c + control Pixy*/
	start_InitI2c_and_I2C_devices_task();

	/*Se astepata initializarea senzorilor.*/
	while(I2C_sensors_initiated == false) vTaskDelay(pdMS_TO_TICKS(50));
#endif
	/*Initializarea modulului WiFi si pornirea task-ului pentru
	 * receptionarea comenzilor de la user.*/
	start_NetworkInit_and_network_task();
#if (MOTOR_MOCK_TEST == OFF)
	/*Se asteapta primirea mesajului corespunzator pentru ACK.*/
	while(allowed_to_send == false) vTaskDelay(pdMS_TO_TICKS(50));
#endif
	/*Pornirea task-ului de */
	start_PID_task();
#if (PIXY_DETECTION == OFF)
	start_UartInit_and_UartJetson_task();
#endif


}
