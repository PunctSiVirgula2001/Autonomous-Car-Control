#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "MotorAndServoControl.h"
#include "PID.h"
#include "Network.h"
extern char rx_buffer[128];
//Autonomous + diagnostic modes
TaskHandle_t myTaskHandle_Autonomous = NULL;
TaskHandle_t myTaskHandle_diagnostic = NULL;

extern QueueHandle_t carControlQueue; // used for general incoming commands from the phone app
extern QueueHandle_t pulse_encoderQueue; // holds the trasnformed time pulse differences incomming from encoder.
extern QueueHandle_t speed_commandQueue;
extern QueueHandle_t PID_commandQueue;
extern QueueSetHandle_t QueueSet;
//#define ESP_LOGI(a,b) printf(b);
void app_main(void) {
	carControlQueue = xQueueCreate(QUEUE_SIZE_CAR_COMMANDS,
			QUEUE_SIZE_DATATYPE_CAR_COMMANDS);
	pulse_encoderQueue = xQueueCreate(QUEUE_SIZE_ENCODER_PULSE,
			QUEUE_SIZE_DATATYPE_ENCODER_PULSE);
	speed_commandQueue = xQueueCreate(QUEUE_SIZE_SPEED, QUEUE_SIZE_DATATYPE_SPEED);
	PID_commandQueue = xQueueCreate(5, sizeof(CarCommand));

	/*Create a set which holds data from all necessary queues.*/
	QueueSet = xQueueCreateSet(
			QUEUE_SIZE_CAR_COMMANDS + QUEUE_SIZE_ENCODER_PULSE);
	configASSERT(speed_commandQueue);
	configASSERT(pulse_encoderQueue);
	configASSERT(speed_commandQueue);
	configASSERT(QueueSet);
	xQueueAddToSet(speed_commandQueue, QueueSet);
	xQueueAddToSet(pulse_encoderQueue, QueueSet);
	xQueueAddToSet(PID_commandQueue, QueueSet);
	start_network_readBuffer_tasks();
	carControl_init();
	configureEncoderInterrupts();
	xTaskCreatePinnedToCore(PIDTask, "PIDTask", 4096, NULL, 10, NULL, 1U);

}

//	nvs_flash_init();
//	init_servo_pwm();
//	init_motor_pwm();
//	changeSTEER(50);
//	changeMotorSpeed(100);
//	configureEncoderInterrupts();
//	// Report counter value
//	PID_Init(&motorPID, 1.2, 0.8, 0.002); //1 0.8 0.0001
//	motorPID.setpoint = 0;

//	xTaskCreate(handleJetsonState, "stateJetson", 2048, NULL, 1, NULL);

//    handler1 = init_mcpwm(FREQUENCY, DEAD_TIME, Driver1High, Driver1Low);
//    handler2 = init_mcpwm(FREQUENCY, DEAD_TIME, Driver2High, Driver2Low);

//  xTaskCreate(controlMotor, "Run PWM", 2048, NULL, 1, &controlMotorHandle);

//    configInterruptEncoder();
//
//
//    while(1)
//    {
//    	printf("%lu \n",frequency);
//    	vTaskDelay(pdMS_TO_TICKS(1000));
//    }
//	changeSTEER(50);
//	vTaskDelay(pdMS_TO_TICKS(3000));
//	changeMotorSpeed(140);
//	vTaskDelay(pdMS_TO_TICKS(3000));
//	changeMotorSpeed(50);
//	vTaskDelay(pdMS_TO_TICKS(3000));
//	changeMotorSpeed(135);
//	vTaskDelay(pdMS_TO_TICKS(3000));
//	changeMotorSpeed(50);

//changeDIRECTION(BREAKING);
//init_uart();
//	read_uart();
//	xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10,
//			NULL);

