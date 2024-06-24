#ifndef MOTORANDSERVOCONTROL
#define MOTORANDSERVOCONTROL

#include "drivers/DcMotor.h"
#include "drivers/ServoMotor.h"
#include "drivers/encoder.h"

/*Car control*/
#define QUEUE_SIZE_CAR_COMMANDS 20
#define QUEUE_SIZE_DATATYPE_CAR_COMMANDS (sizeof(char*))

/*COMMANDS RECEIVED FROM APP*/
typedef enum {
    StopReceived = 0,      		// 00
    ForwardReceived = 1,        // 01
    BackwardReceived = 2,       // 02 //03 ->left light, 04 -> right light
    SpeedReceived = 5,          // 05
    SteerReceived = 6,          // 06
	AutonomousReceived = 7,		// 07
    PID_Changed = 8
} ReceivedState_app;

typedef struct {
    ReceivedState_app command;
    int command_value;
    bool has_value; // Indicates whether command_value is valid
    float KP;
    float KI;
    float KD;
} CarCommand;

/*Car control tasks*/
void CarControlTask(void *pvParameters);
void steer_task(void *pvParameters);

/*Car control functions*/
void carControl_init_and_start_CarControl_task();
void carControl_Backward_init(int break_value);
CarCommand parseCommand(const char* commandStr);

#endif
