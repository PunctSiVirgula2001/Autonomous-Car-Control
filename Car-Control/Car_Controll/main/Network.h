#include <stdio.h>
#include <stdlib.h>
#include "esp_task_wdt.h"
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
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "errno.h"
#include <stdlib.h>


#define WIFI_SSID "Car_Ac_Tuiasi"
#define WIFI_PASS "123456789"
#define MAX_STA_CONN 4  /*Number of maximum connections allowed on access point. */
#define PORT 80 		/*Port at which the access point has been started. */
#define TAG_send "SocketSender"
#define INBUILT_LED_CONNECTED GPIO_NUM_2

/*COMMANDS TO SEND TO APP*/
typedef enum {
    MEASURED_VALUE,
    I_TERM_VALUE,
    OBSTACLE_DETECTED_VALUE,
	ERROR_PID_VALUE,
	ACTUAL_TIME_OF_SEND,
	TEMPRATURE,
    STATE_MAX
} SendCommandType_app;

typedef enum data_type_to_send
{
	INT,
	FLOAT,
	DOUBLE
}data_type_to_send;


//utility function for strings
char* to_string(void *value, data_type_to_send type);

//static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init_softap();
void udp_server_task(void *pvParameters);
void start_network_task();
void sendMessage(int sock, const char *message, struct sockaddr_in6 *addr, socklen_t addr_len);
void HLD_SendMessage(const char* message);
void sendCommandApp(SendCommandType_app commandType, void* commandValue, data_type_to_send type);
void config_Connected_led();
void turnOnLED_connected();
void complement_connected_led();
void blink_led_task(void *pvParameters);
