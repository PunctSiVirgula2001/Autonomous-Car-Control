#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"  // Required for mutex
#include "driver/adc.h"
#include "RemoteWifiControl.h"
#include "ServoControl.h"
#include "esp_task_wdt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "MotorControl_mcpwm.h"
#include "PID.h"


//Autonomous + diagnostic modes
TaskHandle_t myTaskHandle_Autonomous = NULL;
TaskHandle_t myTaskHandle_diagnostic = NULL;

//PID
PID_t motorPID;
float deltaTime = 0.02;  // 20 ms = 0.02 seconds

//encoder
#define EXAMPLE_PCNT_HIGH_LIMIT 2000000000
#define EXAMPLE_PCNT_LOW_LIMIT  -2000000000
#define encoderGPIO_A 35
#define encoderGPIO_B 14


QueueHandle_t queue;
pcnt_unit_handle_t pcnt_unit = NULL;

void setPIDParameters() { // function for reading from keyboard
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

			printf("PID parameters updated: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", kp,
					ki, kd);
		}
	}
	//}
}

extern char bufferReceived[128];
unsigned char SPEED = 0;
unsigned char previousSPEED = 0;
unsigned char steer = 50;

mcpwm_handles_t handler1; // MOSFET DRIVER 1
mcpwm_handles_t handler2; // MOSFET DRIVER 2

TaskHandle_t controlMotorHandle = NULL;
TaskHandle_t controlSteerHandle = NULL;

commandReceived_app mainCommand;

void controlMotor_mcpwm(void *pvParameters) {
	uint32_t notificationValue;
	while (1) {
		notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (notificationValue == FORWARD) {
			motorControl_mcpwm(FORWARD, 100 - SPEED, handler1, handler2);
		} else if (notificationValue == BACKWARD) {
			motorControl_mcpwm(BACKWARD, 100 - SPEED, handler1, handler2);
		} else if (notificationValue == BREAKING) {
			motorControl_mcpwm(BREAKING, 100, handler1, handler2);
		} else if (notificationValue == STOP) {
			motorControl_mcpwm(STOP, 100, handler1, handler2);
		}
	}
}

/* Encoder */
static bool example_pcnt_on_reach(pcnt_unit_handle_t unit,
		const pcnt_watch_event_data_t *edata, void *user_ctx) {
	BaseType_t high_task_wakeup;
	QueueHandle_t queue = (QueueHandle_t) user_ctx;
	// send event data to queue, from this interrupt callback
	xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
	return (high_task_wakeup == pdTRUE);
}
void configureEncoderInterrupts() {

	queue = xQueueCreate(10, sizeof(int));
	pcnt_unit_config_t unit_config = { .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
			.low_limit = EXAMPLE_PCNT_LOW_LIMIT, };

	ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

	pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = 1000, };
	ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

	pcnt_chan_config_t chan_a_config = { .edge_gpio_num = encoderGPIO_B,
			.level_gpio_num = encoderGPIO_A, };
	pcnt_channel_handle_t pcnt_chan_a = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
	pcnt_chan_config_t chan_b_config = { .edge_gpio_num = encoderGPIO_A,
			.level_gpio_num = encoderGPIO_B, };
	pcnt_channel_handle_t pcnt_chan_b = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

	ESP_ERROR_CHECK(
			pcnt_channel_set_edge_action(pcnt_chan_a,
					PCNT_CHANNEL_EDGE_ACTION_DECREASE,
					PCNT_CHANNEL_EDGE_ACTION_INCREASE));
	ESP_ERROR_CHECK(
			pcnt_channel_set_level_action(pcnt_chan_a,
					PCNT_CHANNEL_LEVEL_ACTION_KEEP,
					PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
	ESP_ERROR_CHECK(
			pcnt_channel_set_edge_action(pcnt_chan_b,
					PCNT_CHANNEL_EDGE_ACTION_INCREASE,
					PCNT_CHANNEL_EDGE_ACTION_DECREASE));
	ESP_ERROR_CHECK(
			pcnt_channel_set_level_action(pcnt_chan_b,
					PCNT_CHANNEL_LEVEL_ACTION_KEEP,
					PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

	pcnt_event_callbacks_t cbs = { .on_reach = example_pcnt_on_reach, };

	ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

#if CONFIG_EXAMPLE_WAKE_UP_LIGHT_SLEEP
		       // EC11 channel output high level in normal state, so we set "low level" to wake up the chip
		       ESP_ERROR_CHECK(gpio_wakeup_enable(encoderGPIO_B, GPIO_INTR_LOW_LEVEL));
		       ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
		       ESP_ERROR_CHECK(esp_light_sleep_start());
#endif

}



#define WIFI_SSID "ESP32-Access-Point"
#define WIFI_PASS "123456789"
#define MAX_STA_CONN 4

static const char *TAG = "wifi_softAP";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station MACSTR join, AID=");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station MACSTR leave, AID=");
    }
}

void wifi_init_softap()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             WIFI_SSID, WIFI_PASS);
}

#define PORT 80

static void tcp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    listen(listen_sock, 1);

    while (1) {
        struct sockaddr_in source_addr;
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error or connection closed by client
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
                ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
                // Here, you can handle the received data
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}
//#define ESP_LOGI(a,b) printf(b);
void app_main(void) {
	// Initialize NVS
	    esp_err_t ret = nvs_flash_init();
	    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	        ESP_ERROR_CHECK(nvs_flash_erase());
	        ret = nvs_flash_init();
	    }
	    ESP_ERROR_CHECK(ret);

	    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
	    wifi_init_softap();
	    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);

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

}

