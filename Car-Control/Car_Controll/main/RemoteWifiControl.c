#include "RemoteWifiControl.h"



char bufferReceived[128];
Queue q = {NULL, NULL};
extern commandReceived mainCommand;
const char *ssid = "Mi 10T";
const char *pass = "12345678";
int retry_num = 0;

commandReceived getTwoValues(const char* buffer, int len) {
	commandReceived result = {0, 0}; // Initialize the struct with zeros

    if (len >= 2) {
        // Convert the first two characters to an unsigned char
        char temp[3] = {buffer[0], buffer[1], '\0'};
        result.state = (unsigned char)strtoul(temp, NULL, 10);

        // If buffer has 4 characters, convert the last two characters as well
        if (len == 4) {
            char temp2[3] = {buffer[2], buffer[3], '\0'};
            result.SteerOrSpeed = (unsigned char)strtoul(temp2, NULL, 10);
        }
    }

    return result;
}

commandReceived getTwoValues_2(const char *buffer, int len) {
    commandReceived result = {0, 0};  // Initialize the struct with zeros
    char *spacePos = strchr(buffer, ' ');  // Find the position of the first space character
    char *endLinePos = strchr(buffer, '\n');  // Find the position of the newline character

    // Make sure both the space and the newline characters are found
    if (spacePos != NULL && endLinePos != NULL) {
        char stateStr[20] = {0};  // Temporary string to hold the state value
        char SteerOrSpeedStr[20] = {0};  // Temporary string to hold the SteerOrSpeed value

        // Copy the characters from the start to the space into stateStr
        strncpy(stateStr, buffer, spacePos - buffer);
        // Copy the characters from after the space to the newline into SteerOrSpeedStr
        strncpy(SteerOrSpeedStr, spacePos + 1, endLinePos - spacePos - 1);

        // Convert to unsigned char and store in the struct
        result.state = (unsigned long)strtoul(stateStr, NULL, 10);
        result.SteerOrSpeed = (unsigned long)strtoul(SteerOrSpeedStr, NULL, 10);
    }

    return result;
}

commandReceived_jetson getMultipleValues(const char *buffer, int len) {
    commandReceived_jetson result = {0};  // Initialize the struct with zeros

    char tempBuffer[len + 1];
    strncpy(tempBuffer, buffer, len);
    tempBuffer[len] = '\0';

    char *line = strtok(tempBuffer, "\n");
    while (line != NULL) {
        // Create a copy of the line for inner tokenization
        char lineCopy[strlen(line) + 1];
        strcpy(lineCopy, line);

        // Tokenize the line copy by space and populate the struct
        char *token = strtok(lineCopy, " ");
        if (token != NULL) {
            result.state = strtoul(token, NULL, 10);
        }

        if (result.state == 98) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                result.Steer = strtoul(token, NULL, 10);

            }
        } else if (result.state == 99) {
            result.Steer = 500;
            unsigned long *ptr = &result.class_id;
            while ((token = strtok(NULL, " ")) != NULL) {
            //	printf("aici");
                *ptr = strtoul(token, NULL, 10);
                ptr++;
            }
        }

        // Go to the next line
        line = strtok(NULL, "\n");
    }

    return result;
}


void enqueue(Queue* q, commandReceived data) {
    Node* newNode = (Node*) malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
        return;
    }

    q->rear->next = newNode;
    q->rear = newNode;
}

commandReceived dequeue(Queue* q) {
if (q->front == NULL) {
	printf("Queue is empty. Returning default value.\n");
	commandReceived defaultData = {0, 0};
	return defaultData;
}

Node* temp = q->front;
q->front = q->front->next;

if (q->front == NULL) {
	q->rear = NULL;
}

commandReceived data = temp->data;
free(temp);

return data;
}



void initialize_udp_client() {
    struct sockaddr_in local_addr, remote_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(4240); // The local port to listen on
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on any IP address

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        printf("Unable to create socket: errno %d\n", errno);
        return;
    }
    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        printf("Unable to bind: errno %d\n", errno);
        return;
    }
    while (1) { // Continuous loop to receive messages

        socklen_t remote_addr_len = sizeof(remote_addr);
        int len = recvfrom(sock, bufferReceived, sizeof(bufferReceived) - 1, 0, (struct sockaddr *)&remote_addr, &remote_addr_len);

        // Error occurred during receiving
        if (len < 0) {
            printf("recvfrom failed: errno %d\n", errno);
            break;
        }
        // Data received
        else {
        	bufferReceived[len] = 0; // Null-terminate whatever we received and treat it like a string

        	mainCommand = getTwoValues(bufferReceived, len);
        	printf("%lu %lu\n",mainCommand.state , mainCommand.SteerOrSpeed);
        	enqueue(&q, mainCommand);
        }
    }
    // Close the socket
    shutdown(sock, 0);
}



// Event handler for WiFi events
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        printf("WIFI CONNECTING....\n");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        printf("WiFi CONNECTED\n");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi lost connection\n");
        if (retry_num < 5) {
            esp_wifi_connect();
            retry_num++;
            printf("Retrying to Connect...\n");
        }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        printf("WiFi got IP...\n");
        // Initialize UDP client here
        initialize_udp_client();
    }
}

void wifi_connection() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "Mi 10T",
            .password = "12345678",
        },
    };

    strcpy((char*)wifi_configuration.sta.ssid, ssid);
    strcpy((char*)wifi_configuration.sta.password, pass);

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    printf("wifi_init_softap finished. SSID:%s  password:%s\n", ssid, pass);
}

