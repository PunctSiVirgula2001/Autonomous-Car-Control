#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "MotorControl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"  // Required for mutex
#include "driver/adc.h"
#include "RemoteWifiControl.h"
#include "ServoControl.h"
#include "esp_task_wdt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "sdkconfig.h"
#include "freertos/queue.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

TaskHandle_t myTaskHandle_Autonomous = NULL; //for UART
TaskHandle_t myTaskHandle_diagnostic = NULL;
TaskHandle_t myTaskHandle_jetsonState = NULL;

SemaphoreHandle_t xMutex;



#include "PID.h"
PID_t motorPID;
float deltaTime = 0.02;  // 20 ms = 0.02 seconds


int pulse_count = 0;
int event_count = 0;
int old_count = 0;
int setpoint_pulse_count = 0;


#define EXAMPLE_PCNT_HIGH_LIMIT 2000000000
#define EXAMPLE_PCNT_LOW_LIMIT  -2000000000

#define EXAMPLE_EC11_GPIO_B 35
#define EXAMPLE_EC11_GPIO_A 14
QueueHandle_t queue;
pcnt_unit_handle_t pcnt_unit = NULL;


#define ECHO_TEST_TXD 17
#define ECHO_TEST_RXD 16
#define ECHO_TEST_RTS UART_PIN_NO_CHANGE
#define ECHO_TEST_CTS UART_PIN_NO_CHANGE

#define ECHO_UART_PORT_NUM UART_NUM_2
#define ECHO_UART_BAUD_RATE 115200
#define ECHO_TASK_STACK_SIZE 2048

#define BUF_SIZE (1024)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void setPIDParameters() {
    char buffer[256];
    //while (1) {
       // printf("Enter PID values in the format: set kp ki kd\n");

        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            char *command = strtok(buffer, " ");
            if (strcmp(command, "set") == 0) {
                float kp = atof(strtok(NULL, " "));
                float ki = atof(strtok(NULL, " "));
                float kd = atof(strtok(NULL, " "));

                // Update the PID parameters
                motorPID.Kp = kp;
                motorPID.Ki = ki;
                motorPID.Kd = kd;

                printf("PID parameters updated: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", kp, ki, kd);
            }
        }
    //}
}



//#include <encoder.h>

extern char bufferReceived[128];
SemaphoreHandle_t stateMutex; // Mutex for 'state' // @suppress("Type cannot be resolved")
unsigned char SPEED = 0;
unsigned char previousSPEED = 0;
unsigned char steer = 50;

mcpwm_handles_t handler1; // MOSFET DRIVER 1
mcpwm_handles_t handler2; // MOSFET DRIVER 2

TaskHandle_t controlMotorHandle = NULL;
TaskHandle_t controlSteerHandle = NULL;

commandReceived mainCommand;

#define ANALOG_PIN 33

extern Queue q;  // Initialize an empty queue



void controlMotor(void *pvParameters) {
	uint32_t notificationValue;
	while (1) {
		notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (notificationValue == FORWARD) {
			motorControl(FORWARD, 100 - SPEED, handler1, handler2);
		} else if (notificationValue == BACKWARD) {
			motorControl(BACKWARD, 100 - SPEED, handler1, handler2);
		} else if (notificationValue == BREAKING) {
			motorControl(BREAKING, 100, handler1, handler2);
		} else if (notificationValue == STOP) {
			motorControl(STOP, 100, handler1, handler2);
		}
	}
}

unsigned char previousSTATE = NONE;
void changeDIRECTION(unsigned char dir) {
	if ((previousSTATE != FORWARD && dir == FORWARD)
			|| (dir == FORWARD && SPEED != previousSPEED)) {
		vTaskDelay(pdMS_TO_TICKS(7));
		xTaskNotify(controlMotorHandle, FORWARD, eSetValueWithOverwrite);
		previousSTATE = FORWARD;
		previousSPEED = SPEED;
		printf("Aici %d\n", SPEED);
	}
	if ((previousSTATE != BACKWARD && dir == BACKWARD)
			|| (dir == BACKWARD && SPEED != previousSPEED)) {
		vTaskDelay(pdMS_TO_TICKS(4));
		xTaskNotify(controlMotorHandle, BACKWARD, eSetValueWithOverwrite);
		previousSTATE = BACKWARD;
		previousSPEED = SPEED;
	}
	if (previousSTATE != BREAKING && dir == BREAKING) {
		vTaskDelay(pdMS_TO_TICKS(4));
		xTaskNotify(controlMotorHandle, BREAKING, eSetValueWithOverwrite);
		previousSTATE = BREAKING;
		previousSPEED = 0;
	}
	if (previousSTATE != STOP && dir == STOP) {
		vTaskDelay(pdMS_TO_TICKS(4));
		xTaskNotify(controlMotorHandle, STOP, eSetValueWithOverwrite);
		previousSTATE = STOP;
		previousSPEED = 0;
	}
}

//06xx -> Directie
//01 -> fata
//02 -> spate
//00 -> BREAKING/STOP
//05xx -> VITEZA
unsigned char lastSpeed = 0;
void processState(commandReceived receivedCommand) {
	static unsigned char lastDIR = NONE;
	switch (receivedCommand.state) {
	case StopReceived:
		changeDIRECTION(BREAKING);
		//printf("Breaking \n");
		lastDIR = BREAKING;
		break;
	case ForwardReceived:
		SPEED = receivedCommand.SteerOrSpeed;
		changeDIRECTION(FORWARD);
		//printf("Forward \n");
		lastDIR = FORWARD;
		break;
	case BackwardReceived:
		SPEED = receivedCommand.SteerOrSpeed;
		changeDIRECTION(BACKWARD);
		//printf("Backward \n");
		lastDIR = BACKWARD;
		break;
	case SpeedReceived:
		SPEED = receivedCommand.SteerOrSpeed;
		changeDIRECTION(lastDIR);
		//printf("Dir \n");
		break;
	default:
		// Handle unexpected states
		break;
	}
}

void echo_task(void *arg);

int dir = 0;
int last_speed = 0;
float controlOutput =0;
void handleJetsonState(void *arg);
void queueHandlerTask(void *pvParameters) { // Diagnostic MODE
    // Initialize PID controller
   // PID_Init(&motorPID, 1.0, 0.1, 0.01);
    motorPID.setpoint = 0;  // initial setpoint

    while (1) {
        // Check and process queue data
    	commandReceived val;
    	val.SteerOrSpeed = 0;
    	val.state = 0;

        if (q.front != NULL) {
        	if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            val = dequeue(&q);
        	}
        	// Release the mutex
        	 xSemaphoreGive(xMutex);
        	// printf("Diagnostic task. %lu \n", val.SteerOrSpeed);
            // Your existing logic
            if (val.state == SteerReceived)
                changeSTEER(val.SteerOrSpeed);
            else if(val.state == StopReceived){
                motorPID.setpoint = 0;
                motorPID.integral =0;
            }
            else if(val.state == ForwardReceived)
            {
                motorPID.setpoint = mapValue_pid(last_speed+100);
                motorPID.integral =0;
                dir = 1;
            }
            else if(val.state == BackwardReceived)
            {
                motorPID.setpoint =  mapValue_pid(100-last_speed);
                motorPID.integral =0;
                dir = -1;
            }
            else if(val.state == SpeedReceived)
            {
                motorPID.setpoint = mapValue_pid(100+dir*val.SteerOrSpeed);
                last_speed = val.SteerOrSpeed;
            }
            else if(val.state == AutonomousReceived){
            	xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 1,&myTaskHandle_Autonomous);
            	xTaskCreate(handleJetsonState, "jetsonState", ECHO_TASK_STACK_SIZE, NULL, 1,&myTaskHandle_jetsonState);
            	vTaskDelay(pdMS_TO_TICKS(200));
            	vTaskDelete(NULL);
            }
            // Measure current motor speed --> in speedReadtask.
            // Compute PID output
        }

        vTaskDelay(pdMS_TO_TICKS(8));
    }
}



void speedReadtask(void *pvParameters) // worksfor both autonoumous and diagnostic
{
	   while (1) {
		   old_count = pulse_count;
		   ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
		 //  printf("Pulse count: %d \n", 5*(pulse_count-old_count));
		   motorPID.measured = 5*(pulse_count-old_count);
		   setPIDParameters();
		   controlOutput = PID_Compute(&motorPID, deltaTime); //-1024 - 1024
		   controlOutput = reverseMapValue_pid(controlOutput); //0 - 200
		   if(controlOutput>160)
			   controlOutput=160;
		   if(controlOutput <130 && controlOutput>110)
			   controlOutput=132;

		              // Apply the PID output (ensure it's within -100 and 100)
		              //int newSpeed = (int) fminf(fmaxf(controlOutput, -100.0), 100.0);
		    printf("Control %f\n", controlOutput);
		    changeMotorSpeed(controlOutput);
		   vTaskDelay(pdMS_TO_TICKS(200));

	   }
}




float mapValue(float value) {
    float inputStart = 0.0;
    float inputEnd = 1024.0;
    float outputStart = 0.0;
    float outputEnd = 100.0;

    float mappedValue = (value - inputStart) / (inputEnd - inputStart) * (outputEnd - outputStart) + outputStart;

    return mappedValue;
}

uart_config_t uart_config;
uint8_t *data;
commandReceived_jetson jetson;
void uart_init()
{
	uart_config_t uart_config = { .baud_rate = ECHO_UART_BAUD_RATE, .data_bits =
			UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits =
			UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
			.source_clk = UART_SCLK_DEFAULT, };

	// Install UART driver
	ESP_ERROR_CHECK(
			uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));

	// Configure UART parameters
	ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));

	// Set UART pins
	ESP_ERROR_CHECK(
			uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

	// Configure a temporary buffer for the incoming data
	data = (uint8_t*) malloc(BUF_SIZE);
	jetson.Steer=0;
	jetson.state=0;
	uint8_t tmp_buf[BUF_SIZE];
	    int len;
	do {
	        len = uart_read_bytes(ECHO_UART_PORT_NUM, tmp_buf, BUF_SIZE, 20 / portTICK_PERIOD_MS);
	    } while (len > 0);
}

uint8_t countSameState = 0;
uint8_t lastSTATE = 0;
bool state_activated = false;
const uint8_t state_threshold = 5;  // Threshold for stopping
uint8_t no_command =0;

uint64_t ignore_start_time = 0;  // Time when we start ignoring StopJetson states
//int parked = 0;
void handleJetsonState(void *pvParameters) {
    // Reset counter and flags if state changes
	while(1){

    if (jetson.class_id != lastSTATE) {
        countSameState = 0;
        state_activated = false;
    }

    // Update the last state
    lastSTATE = jetson.class_id;
    jetson.class_id = DrivingJetson;
    switch (jetson.class_id) {

        case StopJetson:
            if (!state_activated) {
            	countSameState++;
                if (countSameState >= state_threshold) {
                     printf("Handling StopJetson state. Stopping the car.\n");
                    state_activated = true;
                    vTaskDelay(pdMS_TO_TICKS(100));
                    motorPID.integral =0;
                    motorPID.setpoint=0;
                    // Insert code to actually stop the car here
                    vTaskDelay(pdMS_TO_TICKS(3000));

                } else {
                    printf("StopJetson detected but threshold not reached yet.\n");
                }
            } else {
            	motorPID.setpoint = mapValue_pid(140);
            }
            break;
        case GreenJetson:
		  if (!state_activated) {
			  countSameState++;
			  if (countSameState >= state_threshold) {
				  state_activated = true;
				  printf("Handling GreenJetson state. Doing something.\n");
				  state_activated = true;
				  motorPID.setpoint = mapValue_pid(140);
				  // Insert code to handle GreenJetson state
			  } else {
				  printf("GreenJetson detected but threshold not reached yet.\n");
			  }
		  } else {
			  printf("Ignoring additional GreenJetson states.\n");
		  }
		  break;
        case ParkingJetson:
        	if (!state_activated) {
			  countSameState++;
			  if (countSameState >= state_threshold) {
				  state_activated = true;
				  printf("Handling ParkingJetson state. Doing something.\n");
				  vTaskDelay(pdMS_TO_TICKS(400));
				  motorPID.integral =0;
				  motorPID.setpoint=0;
				  //   =1;

				  // Insert code to handle GreenJetson state
			  } else {
				  printf("ParkingJetson detected but threshold not reached yet.\n");
			  }
		  } else {
			  printf("Ignoring additional ParkingJetson states.\n");
		  }
		  break;
        case ChargingStationJetson:
            printf("Handling ChargingStationJetson state.\n");
            // Insert code to handle ChargingStationJetson state
            break;
        case PedestrianCrossingJetson:
        	if (!state_activated) {
			  countSameState++;
			  if (countSameState >= state_threshold) {

//				  if(motorPID.measured >0){
//					  vTaskDelay(pdMS_TO_TICKS(1500));
//					  motorPID.integral =0;
//					  motorPID.setpoint=0;
//				  }
//				  else{
				  printf("Handling ChargingStationJetson state. Doing something.\n");
				  state_activated = true;
				  vTaskDelay(pdMS_TO_TICKS(200));
				  motorPID.integral =0;
				  motorPID.setpoint=0;
				  vTaskDelay(pdMS_TO_TICKS(1500));
				  //}
				  // Insert code to handle GreenJetson state
			  } else {
				  printf("PedestrianCrossingJetson detected but threshold not reached yet.\n");
			  }
		  } else {
			  motorPID.setpoint = mapValue_pid(140);
			  printf("Ignoring additional PedestrianCrossingJetson states.\n");
		  }
		  break;
        case RedJetson:
            printf("Handling RedJetson state.\n");
            if (!state_activated) {
			  countSameState++;
			  if (countSameState >= state_threshold) {
				  printf("Handling RedJetson state. Doing something.\n");
				  state_activated = true;
				  vTaskDelay(pdMS_TO_TICKS(1500));
				  motorPID.integral =0;
				  motorPID.setpoint=0;
				  //vTaskDelay(pdMS_TO_TICKS(1000));
				  // Insert code to handle GreenJetson state
			  } else {
				  printf("RedJetson detected but threshold not reached yet.\n");
			  }
		  } else {
			  printf("Ignoring additional RedJetson states.\n");
		  }
		  break;
        case YellowJetson:
            printf("Handling YellowJetson state.\n");
            if (!state_activated) {
			  countSameState++;
			  if (countSameState >= state_threshold) {
				  printf("Handling YellowJetson state. Doing something.\n");
				  vTaskDelay(pdMS_TO_TICKS(1500));
				  state_activated = true;
				  motorPID.integral =0;
				  motorPID.setpoint=0;
				  // Insert code to handle GreenJetson state
			  } else {
				  printf("YellowJetson detected but threshold not reached yet.\n");
			  }
		  } else {
			  printf("Ignoring additional YellowJetson states.\n");
		  }
		  break;
        case DrivingJetson:
        	//if(parked==0){
				  printf("Handling DrivingJetson state. Doing something.\n");
//				  motorPID.setpoint = mapValue_pid(145);
				  changeMotorSpeed(500);
        	//}
		  break;
        default:
            printf("Received unknown state.\n");
            break;
    		}

	vTaskDelay(pdMS_TO_TICKS(10));
	}
}


void echo_task(void *arg) { // Autonomous MODE.
	// Configure parameters of the UART driver
	//motorPID.setpoint = 0;
	printf("Autonomous task. \n");
	// motorPID.setpoint =  mapValue_pid(140);
	//parked=0;
	while (1) {
		 commandReceived val;
		 val.SteerOrSpeed=0;
		 val.state=0;
		// Read data from the UART
		if (q.front != NULL) {
			if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
			{
			      val = dequeue(&q);
			}
			// Release the mutex
			xSemaphoreGive(xMutex);
		      if(val.state != AutonomousReceived)
		      {

		    	  xTaskCreate(queueHandlerTask, "QueueHandlerTask", 2048, NULL, 1, &myTaskHandle_diagnostic);
		    	  vTaskDelay(pdMS_TO_TICKS(200));
		    	  vTaskDelete(myTaskHandle_jetsonState);
		    	  vTaskDelete(NULL);
		      }
		}
		int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1),
				20 / portTICK_PERIOD_MS);
		// state machine for what comes from jetson through uart
		if (len > 0)
		{

			data[len] = '\0';
			jetson = getMultipleValues((char*) data, len);
			//printf("Received DATA: %s\n", data);
			//if(jetson.state == 98){ //steering

			jetson.class_id = DrivingJetson;
			printf("Received: %d %lu %lu\n", (int)(mapValue(jetson.Steer)), jetson.state, jetson.class_id); // You can also use ESP_LOGI(TAG, "Received: %s", data);
			changeSTEER(1.2*(100-(int)(mapValue(jetson.Steer))));

			//}
				no_command =1;
				while (strchr((char *)data, '\n') != NULL && len > 8) {
					char *newline_pos = strchr((char *)data, '\n');  // Find the first newline character
					    if (newline_pos != NULL) {
					        int offset = newline_pos - (char *)data + 1;  // Calculate the offset to the character after the newline
					        if (offset <= len) {  // Make sure we don't go out of bounds
					            memmove(data, newline_pos + 1, len - offset);  // Shift the remaining characters to the beginning
					            len -= offset;  // Update the length

					            // Only process if remaining length is sufficiently large
					            if (len >= 8) {
					                jetson = getMultipleValues((char*) data, len);
					               printf("Received: %d %lu %lu %lu %lu %lu %lu %lu\n", (int)(mapValue(jetson.Steer)), jetson.state, jetson.class_id, jetson.class_confidence, jetson.det_confidence, jetson.x, jetson.y, jetson.width);
					               no_command =0;
					            //   handleJetsonState();
					            }
					        }
					    }

			    }
				//if(no_command == 1)
					//handleJetsonState();
			//printf("%d \n", len);

		} else if (len == -1) {
			printf("UART read error.\n");
		}
	}
}



static bool example_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    // send event data to queue, from this interrupt callback
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}


void configureEncoderInterrupts()
{

	queue= xQueueCreate(10, sizeof(int));
	pcnt_unit_config_t unit_config = {
		        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
		        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
		    };

		    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

		    pcnt_glitch_filter_config_t filter_config = {
		        .max_glitch_ns = 1000,
		    };
		    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

		    pcnt_chan_config_t chan_a_config = {
		        .edge_gpio_num = EXAMPLE_EC11_GPIO_A,
		        .level_gpio_num = EXAMPLE_EC11_GPIO_B,
		    };
		    pcnt_channel_handle_t pcnt_chan_a = NULL;
		    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
		    pcnt_chan_config_t chan_b_config = {
		        .edge_gpio_num = EXAMPLE_EC11_GPIO_B,
		        .level_gpio_num = EXAMPLE_EC11_GPIO_A,
		    };
		    pcnt_channel_handle_t pcnt_chan_b = NULL;
		    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

		    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
		    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
		    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
		    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

		       pcnt_event_callbacks_t cbs = {
		           .on_reach = example_pcnt_on_reach,
		       };

		       ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));

		       ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
		       ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
		       ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

		   #if CONFIG_EXAMPLE_WAKE_UP_LIGHT_SLEEP
		       // EC11 channel output high level in normal state, so we set "low level" to wake up the chip
		       ESP_ERROR_CHECK(gpio_wakeup_enable(EXAMPLE_EC11_GPIO_A, GPIO_INTR_LOW_LEVEL));
		       ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
		       ESP_ERROR_CHECK(esp_light_sleep_start());
		   #endif
}
//#define ESP_LOGI(a,b) printf(b);
void app_main(void) {
		xMutex = xSemaphoreCreateMutex();
	    nvs_flash_init();
	    wifi_connection();
		init_pwm();
		init_motor_pwm();
		uart_init();
		changeSTEER(50);
		changeMotorSpeed(100);
		configureEncoderInterrupts();
	       // Report counter value
		PID_Init(&motorPID, 1.2, 0.8, 0.002);//1 0.8 0.0001
		motorPID.setpoint = 0;


		xTaskCreate(queueHandlerTask, "QueueHandlerTask", 2048, NULL, 1,  &myTaskHandle_diagnostic);
		xTaskCreate(speedReadtask, "speedReadtask", 2048, NULL, 1, NULL);
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

}


