#include "Network.h"
bool allowed_to_send = false;
char rx_buffer[128];
static const char *TAG = "wifi_softAP";
struct sockaddr_in6 source_addr_global; // For IPv4 or IPv6
socklen_t addr_len_global;
int sock_global; 						// Socket descriptor
char* stateSendToAppStrings[STATE_MAX] = {
    "MEASURED_VALUE", 			// Corresponds to MEASURED_VALUE
    "I_TERM_VALUE",     		// Corresponds to I_TERM_VALUE
    "OBSTACLE_DETECTED_VALUE",   // Corresponds to OBSTACLE_DETECTED_VALUE
	"ERROR_PID_VALUE",			// Corresponds to ERROR_PID_VALUE
	"ACTUAL_TIME_OF_SEND",
	"TEMP_VALUE",
	"DistSensFw",
	"DistSensBw",
	"ADXL_ROLL",
	"ADXL_PITCH",
	"NEWLINE_LOG_INFO"
};

// * 00 - measured speed
// * 01 - setpoint
// * 02 - temperature
// * 03 - obstacoll detected
// * 04 - I_term
// * 05 - error_pid


//Initialize a handler for blinking led
TaskHandle_t handlerBlinkLedTask = NULL;


void wifi_init_softap() {
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_config_t wifi_config = {
	  .ap = { .ssid = WIFI_SSID,
	  .ssid_len = strlen(WIFI_SSID),
	  .password = WIFI_PASS,
	  .max_connection = MAX_STA_CONN,
	  .authmode = WIFI_AUTH_WPA_WPA2_PSK },
	};
	if (strlen(WIFI_PASS) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	//ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", WIFI_SSID,
			//WIFI_PASS);
}
extern QueueHandle_t diagnosticModeControlQueue;


static void wait_for_ack(int sock) {
    while (1) {
        // Primirea datelor
        struct sockaddr_in6 source_addr; // Suficient de mare pentru IPv4 sau IPv6
        socklen_t addr_len = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                           (struct sockaddr*)&source_addr, &addr_len);

        // Verificarea erorilor sau datelor primite
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            continue; // Continuă bucla în cazul unei erori
        } else {
            rx_buffer[len] = '\0'; // Terminarea șirului de caractere primit
            const char *message = rx_buffer;

            // Verificarea mesajului "ACK 9999"
            if (strcmp(message, "ACK 9999") == 0) {
                const char *response = "YES"; // Răspuns specific pentru "ACK 9999"

                // Actualizarea variabilelor globale cu adresa sursă și lungimea adresei
                source_addr_global = source_addr;
                addr_len_global = addr_len;
                sock_global = sock;

                // Trimiterea răspunsului
                sendMessage(sock, response);

                allowed_to_send = true;

                // Distrugerea task-ului pentru LED-ul intermitent
                if (handlerBlinkLedTask) {
                    vTaskDelete(handlerBlinkLedTask);
                }
                turnOnLED_connected();
                return; // Ieșire din buclă după primirea mesajului
            }
        }
    }
}

static void handle_received_message(const char *message, int sock, struct sockaddr_in6 source_addr, socklen_t addr_len) {
    // Actualizarea variabilelor globale
    source_addr_global = source_addr;
    addr_len_global = addr_len;
    sock_global = sock;

    // Gestionarea mesajului primit
    if (allowed_to_send == true) {
        if (xQueueSend(diagnosticModeControlQueue, &message, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "Failed to buffer the incoming data from app.");
        }
    }
}

// Funcție pentru trimiterea unui mesaj printr-un socket
void sendMessage(int sock, const char *message) {
    if (sendto(sock, message, strlen(message), 0, (struct sockaddr*)&source_addr_global, addr_len_global) < 0) {
        ESP_LOGE(TAG, "Failed to send message: %s, errno %d", message, errno);
        vTaskDelay(pdMS_TO_TICKS(50));
    } else {
       // ESP_LOGI(TAG, "Message sent: %s", message);
    }
}



void udp_server_task(void *pvParameters) {
    // Configurarea LED-ului de conectare
    config_Connected_led();

    // Crearea task-ului pentru LED-ul intermitent
    xTaskCreatePinnedToCore(blink_led_task, "blink_led_task", 2048, NULL, 5,
                            &handlerBlinkLedTask, 0U);

    // Configurarea adresei și a protocolului
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_UDP;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Legare la orice adresă
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    // Crearea socket-ului
    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    // Legarea socket-ului la adresa specificată
    int err = bind(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    // Așteptarea pentru mesajul "ACK 9999"
    wait_for_ack(sock);

    while (1) {
        // Primirea datelor
        struct sockaddr_in6 source_addr; // Suficient de mare pentru IPv4 sau IPv6
        socklen_t addr_len = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                           (struct sockaddr*)&source_addr, &addr_len);

        // Verificarea erorilor sau datelor primite
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        } else {
            rx_buffer[len] = '\0'; // Terminarea șirului de caractere primit
            handle_received_message(rx_buffer, sock, source_addr, addr_len);
        }
    }

    // Închiderea socket-ului
    if (sock != -1) {
        ESP_LOGI(TAG, "Shutting down socket");
        shutdown(sock, 0);
        close(sock);
    }
}



char* to_string(void* value, data_type_to_send type) {
    char* strValue = malloc(20 * sizeof(char));
    if (strValue == NULL) {
        // Handle memory allocation failure if needed
        return NULL;
    }

    // Convert the value to a string based on its type
    switch (type) {
        case INT:
            sprintf(strValue, "%d", *(int*)value);
            break;
        case DOUBLE:
            sprintf(strValue, "%.2lf", *(double*)value);
            break;
        case FLOAT:
            sprintf(strValue, "%.2f", *(float*)value);
            break;
    }
    return strValue;
}

void HLD_SendMessage(const char *message) {
	sendMessage(sock_global, message);
}

void sendCommandApp(SendCommandType_app commandType, void* commandValue, data_type_to_send type)
{
	if(allowed_to_send == true)
	{
		char* commandTypeStr = stateSendToAppStrings[commandType];
		char* commandValueStr = (char*)to_string(commandValue, type);

		int lengthNeeded = strlen(commandTypeStr) + strlen(commandValueStr) + 2; // 2 = 1 space + 1 null termination
		char* commandToSend = malloc(lengthNeeded);
		sprintf(commandToSend, "%s %s", commandTypeStr, commandValueStr);
		HLD_SendMessage(commandToSend);
		free(commandValueStr);
		free(commandToSend);
	}
}

void start_network_task() {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
	wifi_init_softap();
	xTaskCreatePinnedToCore(udp_server_task, "udp_server", 4096, NULL, 5, NULL,
			0U);
}
void config_Connected_led() {
	gpio_reset_pin(INBUILT_LED_CONNECTED);
	gpio_set_direction(INBUILT_LED_CONNECTED, GPIO_MODE_OUTPUT);
}

void turnOnLED_connected() {
	gpio_set_level(INBUILT_LED_CONNECTED, 1);
}

void complement_connected_led() {
	static unsigned char state = 0;
	gpio_set_level(INBUILT_LED_CONNECTED, state);
	state ^= 1;
}

void blink_led_task(void *pvParameters) {
	while (1) {
		complement_connected_led();
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}
