#include "Network.h"
#include "Queue.h"

char rx_buffer[128];
static const char *TAG = "wifi_softAP";
Queue Queue_receive_from_app;
struct sockaddr_in6 source_addr_global; // For IPv4 or IPv6
socklen_t addr_len_global;
int sock_global; // Socket descriptor

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

static void udp_server_task(void *pvParameters)
{
    queue_init(&Queue_receive_from_app);

    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_UDP;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to any address
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    while (1) {
        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &addr_len);
        source_addr_global = source_addr; // Copy the source address
        addr_len_global = addr_len; // Copy the address length
        sock_global = sock; // Store the socket descriptor

        // Check for errors or no data read
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        } else {
            // Data received
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            char *rx_buffer_pointer = (char *)malloc(strlen(rx_buffer) + 1);
            if (rx_buffer_pointer != NULL) {
                strcpy(rx_buffer_pointer, rx_buffer);
               // ESP_LOGI(TAG, "%s", rx_buffer);
                enqueue_words(&Queue_receive_from_app, rx_buffer_pointer);
                free(rx_buffer_pointer);
            } else {
                ESP_LOGE(TAG, "Failed to allocate memory for rx_buffer_pointer");
            }
        }

        // Optional: Implement a way to break out of this loop or handle socket closure.
    }

    if (sock != -1) {
        ESP_LOGI(TAG, "Shutting down socket");
        shutdown(sock, 0);
        close(sock);
    }
}

static void read_buffer_task(void *pvParameters) {
    // Dequeue and print each word
    while (1) {
        if (!queue_is_empty(&Queue_receive_from_app)) {
            char *word = (char*)queue_dequeue(&Queue_receive_from_app);
            if (word != NULL) {
                ESP_LOGI(TAG, "Received %s", word);
                // Check for the specific message "9999" and send an acknowledgment
                if (strcmp(word, "ACK 9999") == 0) {
                    const char* ackMessage = "YES";
                    // Send acknowledgment back to the sender
                    if (sendto(sock_global, ackMessage, strlen(ackMessage), 0, (struct sockaddr*)&source_addr_global, addr_len_global) < 0) {
                        ESP_LOGE(TAG, "Failed to send ACK: errno %d", errno);
                    } else {
                        ESP_LOGI(TAG, "ACK sent for %s", word);
                    }
                }
                else
                {
					const char *ackMessage = "NO";
					if (sendto(sock_global, ackMessage, strlen(ackMessage), 0,
							(struct sockaddr*) &source_addr_global,
							addr_len_global) < 0) {

					}
                }
                free(word); // Free the dequeued word
            } else {
                ESP_LOGI(TAG, "No word received or queue is empty.");
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}


void init_and_start_network_tasks()
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
	wifi_init_softap();
	xTaskCreate(udp_server_task, "tcp_server", 4096, NULL, 5, NULL);
	xTaskCreate(read_buffer_task, "read_buffer", 4096, NULL, 6, NULL);
}
