#ifndef REMOTEWIFICONTROL
#define REMOTEWIFICONTROL


#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "errno.h"
#include <stdlib.h>

#define SSID "Mi 10T"
#define PASSWORD "123456789"


typedef enum {
    StopReceived = 0,      // 00
    ForwardReceived = 1,      // 01
    BackwardReceived = 2,     // 02
    SpeedReceived = 5,    // 05
    SteerReceived = 6,   // 06
	AutonomousReceived = 7
} ReceivedState_app;

typedef enum {
	StopJetson = 0,
	GreenJetson = 6,
	ParkingJetson = 1,
	ChargingStationJetson = 2,
	PedestrianCrossingJetson = 3,
	RedJetson = 5,
	YellowJetson = 4,
	DrivingJetson = 7
}ReceivedState_jetson;

typedef struct {
    unsigned long state;
    unsigned long SteerOrSpeed;
} commandReceived;

typedef struct {
    unsigned long state;
    unsigned long Steer;
    unsigned long class_id;
    unsigned long class_confidence;
    unsigned long det_confidence;
    unsigned long x;
    unsigned long y;
    unsigned long width;
} commandReceived_jetson;




#endif
