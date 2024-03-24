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
#include "esp_timer.h"
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

//static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init_softap();
void udp_server_task(void *pvParameters);
void send_data_to_app_task(void *pvParameters);
void start_network_readBuffer_tasks();
void sendMessage(int sock, const char *message, struct sockaddr_in6 *addr, socklen_t addr_len);
void HLD_SendMessage(const char* message);
