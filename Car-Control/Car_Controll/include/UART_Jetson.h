
#ifndef MAIN_UART_JETSON_H_
#define MAIN_UART_JETSON_H_

#include "MotorAndServoControl.h"
#include "driver/uart.h"

/*UART Settings */
#define UART_BAUD 115200
#define UART_WORD_LENGTH_BITS UART_DATA_8_BITS
#define UART_PORT UART_NUM_2
#define UART_TX 17
#define UART_RX 16
#define UART_BUFFER_SIZE 1024 * 2
#define UART_Queue_SIZE 20
#define UART_RTS_PIN 18
#define UART_CTS_PIN 19

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
    int Steer;
    int Speed;
    unsigned long class_id;
    unsigned long class_confidence;
    unsigned long det_confidence;
    unsigned long x;
    unsigned long y;
    unsigned long width;
} commandReceived_jetson;

void JetsonUartConfig();
void uart_Jetson_Task (void *params);
void start_UartJetson_task();


#endif /* MAIN_UART_JETSON_H_ */
