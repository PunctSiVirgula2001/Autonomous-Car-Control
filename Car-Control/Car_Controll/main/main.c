#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"  // Required for mutex
#include "driver/adc.h"
#include "RemoteWifiControl.h"
#include "esp_task_wdt.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "MotorAndServoControl.h"
#include "PID.h"
#include "Queue.h"
#include "Network.h"
extern char rx_buffer[128];
extern Queue Queue_receive_from_app;

//Autonomous + diagnostic modes
TaskHandle_t myTaskHandle_Autonomous = NULL;
TaskHandle_t myTaskHandle_diagnostic = NULL;

extern char bufferReceived[128];

//#define ESP_LOGI(a,b) printf(b);
void app_main(void) {
	start_network_readBuffer_tasks();
	carControl_init();


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


