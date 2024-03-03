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

#define WIFI_SSID "ESP32-Access-Point"
#define WIFI_PASS "123456789"
#define MAX_STA_CONN 4
#define PORT 80

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init_softap();
static void udp_server_task(void *pvParameters);
static void read_buffer_task(void *pvParameters);
void init_and_start_network_tasks();