#include "MotorAndServoControl.h"
#include "I2C_cameraSensor.h"
#include "Network.h"

//Queue set which adds all data from all queues.
QueueSetHandle_t QueueSetPIDNecessaryCommands;
extern QueueHandle_t autonomousModeControlPixyQueue;

CarCommand parseCommand(const char *commandStr)
{
	CarCommand cmd;
	memset(&cmd, 0, sizeof(cmd)); // Initialize the structure with zeros
	if (strncmp(commandStr, "08", 2) == 0)
	{
		// Handle command "08" specifically to parse KP, KI, KD values
		sscanf(commandStr, "08%f %f %f", &cmd.KP, &cmd.KI, &cmd.KD);
		cmd.command = PID_Changed; // Set the command to the appropriate enum value
		cmd.has_value = false;	   // Since KP, KI, KD are used, command_value is not relevant
	}
	else if (strlen(commandStr) >= 2)
	{
		char commandPart[3] = {commandStr[0], commandStr[1], '\0'};
		cmd.command = atoi(commandPart);
		// Check if there's more to the command than just the command part
		if (strlen(commandStr) > 2)
		{
			cmd.has_value = true;
			// Extract the command value, which is optional
			char valuePart[5];					   // Assuming the value part will not exceed 4 digits
			strncpy(valuePart, commandStr + 2, 4); // Copy the rest of the string as command value
			valuePart[4] = '\0';				   // Null-terminate the string
			cmd.command_value = atoi(valuePart);
		}
		else
		{
			cmd.has_value = false;
		}
	}
	return cmd;
}

void carControl_Backward_init(int break_value)
{
	changeMotorSpeed(0);
	vTaskDelay(pdMS_TO_TICKS(50));
	changeMotorSpeed(break_value);
	vTaskDelay(pdMS_TO_TICKS(50));
	changeMotorSpeed(0);
	vTaskDelay(pdMS_TO_TICKS(50));
	changeMotorSpeed(break_value);
	vTaskDelay(pdMS_TO_TICKS(50));
}


bool calib_motor_done = false;
void carControl_calibrate_motor()
{
	update_motor_pwm(2000);
	vTaskDelay(pdMS_TO_TICKS(3000));
	update_motor_pwm(1000);
	vTaskDelay(pdMS_TO_TICKS(3000));
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(3000));
	//ESP_LOGI(" ", "ESC Calibration over.");
	calib_motor_done = true;
}

static void mock_forward_and_backward_test_5_seconds()
{

	/* MOVING FORWARD */
	update_motor_pwm(1600);
	vTaskDelay(pdMS_TO_TICKS(5000));

	/*BACKWARD MODE INIT*/
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(1000));
	update_motor_pwm(1400);
	vTaskDelay(pdMS_TO_TICKS(1000));
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(1000));

	/* MOVING BACKWARD */
	update_motor_pwm(1400);
	vTaskDelay(pdMS_TO_TICKS(5000));

	/*STOP*/
	update_motor_pwm(1500);
	vTaskDelay(pdMS_TO_TICKS(5000));
}

static void pwm_from_1550_to_1650_step_10()
{
	static int i = 0;
	i++;
    update_motor_pwm(1540+i);
	vTaskDelay(pdMS_TO_TICKS(10000));
	if(i==60)
		i=0;

}

QueueHandle_t diagnosticModeControlQueue = NULL;
QueueHandle_t autonomousModeControlUartQueue = NULL;
QueueHandle_t speed_commandQueue = NULL;
QueueHandle_t steer_commandQueue = NULL;
QueueHandle_t PID_commandQueue = NULL;
QueueSetHandle_t QueueSetAutonomousOrDiagnostic = NULL;

/* This variable allows the uart_task to fill Autonomous Queue for controlling the car. */
bool AutonomousMode = false;
void carControl_Task(void *pvParameters) {

#if (MOTOR_MOCK_TEST == ON)
	while(1)
	{
		//ESP_LOGI("AICI","fasf");
		//mock_forward_and_backward_test_5_seconds();
		pwm_from_1550_to_1650_step_10();
	}
#elif (MOTOR_MOCK_TEST == OFF)
	CarCommand cmd = { StopReceived, 0, false, 0.0f, 0.0f, 0.0f };
	int last_motor_speed = 0;
	int speed_multiplier = 0;
	int speed = 0;
	char *command;
	QueueSetMemberHandle_t xActivatedModeAorM_Member;

	while (1) {
		//ESP_LOG("CarControl_TASK","%u ms",pdTICKS_TO_MS(xTaskGetTickCount()));
		if (diagnosticModeControlQueue != NULL && (autonomousModeControlUartQueue != NULL || autonomousModeControlPixyQueue != NULL)) {
			xActivatedModeAorM_Member = xQueueSelectFromSet(QueueSetAutonomousOrDiagnostic, portMAX_DELAY);

			if(xActivatedModeAorM_Member == diagnosticModeControlQueue)
			{
				AutonomousMode = false;
				xQueueReceive(xActivatedModeAorM_Member, &command, 0);
				cmd = parseCommand(command);
			}
#if (PIXY_DETECTION == OFF)
			else if(xActivatedModeAorM_Member == autonomousModeControlUartQueue)
			{
				xQueueReceive(xActivatedModeAorM_Member, &command, 0);

				if(strncmp(command,"sp=",3)==0)
				{
					sscanf(command, "sp=%d", &cmd.command_value);
					cmd.command = SpeedReceived;
				}
				else if(strncmp(command,"st=",3)==0)
				{
					sscanf(command, "st=%d", &cmd.command_value);
					cmd.command = SteerReceived;
				}
			}
#elif (PIXY_DETECTION == ON)
			else if(xActivatedModeAorM_Member == autonomousModeControlPixyQueue)
			{
				static uint8_t nrCmd = 0U;
				nrCmd++;
				xQueueReceive(xActivatedModeAorM_Member, &cmd.command_value, 0);
				if(nrCmd == 1U)
				{
					cmd.command = SpeedReceived;
					ESP_LOGI("PIXY","Speed: %d", cmd.command_value);
				}
				else if(nrCmd == 2U)
				{
					cmd.command = SteerReceived;
					ESP_LOGI("PIXY","Steer: %d", cmd.command_value);
					nrCmd = 0U;
				}


			}
#endif


				switch (cmd.command) {
				case StopReceived:
					speed = 0;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					speed_multiplier = 0;
					break;
				case ForwardReceived:
					speed_multiplier = 1;
					speed = speed_multiplier * last_motor_speed;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					HLD_SendMessage("OKFWD!");
					break;
				case BackwardReceived:
					speed_multiplier = -1;
					speed = speed_multiplier * last_motor_speed;
					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					HLD_SendMessage("OKBWD!");
					break;
				case SpeedReceived:
					if(AutonomousMode == false)
						speed = speed_multiplier * cmd.command_value;
					else
						speed = cmd.command_value;

					xQueueSend(speed_commandQueue,&speed,portMAX_DELAY);
					last_motor_speed = cmd.command_value;
					HLD_SendMessage("OKSPEED!");
					break;
				case SteerReceived:
					xQueueSend(steer_commandQueue,&cmd.command_value,portMAX_DELAY);
					HLD_SendMessage("OKSTEER!");
					break;
				case AutonomousReceived:
					AutonomousMode = true;
					ESP_LOGI("Auton", "");
					break;
				case PID_Changed:
					xQueueSend(PID_commandQueue,&cmd,portMAX_DELAY);
					HLD_SendMessage("OKPID!");
					break;
				}

		} else
			vTaskDelay(pdMS_TO_TICKS(250));
	}
#endif
}

void steer_task(void *pvParameters) {
	int steer = 0;
	while (1) {
		if (steer_commandQueue != NULL) {
			if (xQueueReceive(steer_commandQueue, &steer, portMAX_DELAY) == pdPASS) {
				changeSTEER(steer);
			}
		} else
			vTaskDelay(pdMS_TO_TICKS(250));
	}
}

void carControl_init() {

	init_servo_pwm();
	init_motor_pwm();
	update_servo_pwm(1500); // ESC init
	vTaskDelay(pdMS_TO_TICKS(2000));
	update_motor_pwm(1500);
	//carControl_calibrate_motor();
	//while(calib_motor_done!=true) vTaskDelay(pdMS_TO_TICKS(50));
	vTaskDelay(pdMS_TO_TICKS(500)); // wait for init to complete
	xTaskCreatePinnedToCore(carControl_Task, "carControl_task", 4096, NULL, 6,
	NULL, 0U);
	xTaskCreatePinnedToCore(steer_task, "steer_task", 4096, NULL, 5,NULL, 1U);

}

