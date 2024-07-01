/*
 * I2C_sensorControl.c
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#include "I2C_sensorControl.h"

QueueHandle_t I2C_commandQueue;
extern QueueHandle_t autonomousModeControlPixyQueue;
bool I2C_sensors_initiated = false;
I2C_COMMAND i2c_command;
int speed_distance_sens_scaling;
Pixy2Lines lines;
extern bool AutonomousMode;


void I2C_devices_task(void *pvParameters) {
	double temp;
	I2C_sensors_initiated = false;

	// Initialize ESP32 as master.
	I2C_master_init();

	ESP_LOGI("I2C_sensors", "Sensors init start!");
	// Initialize sensors.
	I2C_sensors_initiated = I2C_addAndInitialiseSensors();
	if (I2C_sensors_initiated == false)
		ESP_LOGI("I2C_sensors", "Sensors init failed!");
	else
		ESP_LOGI("I2C_sensors", "Sensors successfully initiated!");

	// Create structure for managing each sensor.
	sensor_t sensors[] =
	{
		{I2C_distance_sens_fw_dev_handle,I2C_distance_sens_fw_mux, distance_sens1, distance_sens1, SENSOR_IDLE },
		{I2C_distance_sens_bw_handle, I2C_distance_sens_bw_mux, distance_sens2, distance_sens2,SENSOR_IDLE },
		{I2C_adxl345_sens_dev_handle, I2C_adxl345_sens_mux, adxl_acc, adxl_acc, SENSOR_IDLE },
		{I2C_pixy2_dev_handle, I2C_pixy2_camera_mux, pixy2, pixy2, SENSOR_IDLE },
		{I2C_temp_sens_dev_handle, I2C_temp_sens_mux, temp_sens, temp_sens, SENSOR_IDLE }
	};

	I2C_COMMAND i2c_command = { 0, 0, 0 };

	size_t sensor_count = sizeof(sensors) / sizeof(sensor_t);
	size_t current_sensor_index = 0;
	sensor_t *current_sensor = &sensors[current_sensor_index];

	while (1) {

		while (current_sensor->tokens == 0) {
			current_sensor_index = (current_sensor_index + 1) % sensor_count;
			current_sensor = &sensors[current_sensor_index];
		}
		static uint16_t readValSensor_dist1_prev = 0;
		static double readValSensor_dist1_final = 0;

		static uint16_t readValSensor_dist2_prev = 0;
		static double readValSensor_dist2_final = 0;

		switch (current_sensor->state) {
		case SENSOR_IDLE:
			I2C_select_multiplexer_channel(current_sensor->mux);
			vTaskDelay(pdMS_TO_TICKS(5)); // Allow for multiplexer switch
			current_sensor->state = SENSOR_READING;
			break;

		case SENSOR_READING:
			switch (current_sensor->mux) {
			case I2C_distance_sens_fw_mux:
				uint16_t readValSensor1;
				readValSensor1 = VL53L0X_readRangeContinuousMillimeters(
						I2C_distance_sens_fw_dev_handle);
				readValSensor_dist1_final = readValSensor1;

				sendCommandApp(DistSensFw, (double*) &readValSensor_dist1_final,
						DOUBLE);
				static int countSendStop_sens1 = 0, countSendStart_sens1 = 0;
				if (readValSensor_dist1_final
						< (Threshold_dist * speed_distance_sens_scaling)
						&& countSendStop_sens1 == 0) {
					countSendStop_sens1++;
					countSendStart_sens1 = 0;
					i2c_command.command = I2C_STOP_MOTOR;
					i2c_command.commandValue = 0;
					i2c_command.sendingSensor = I2C_distance_sens_fw_dev_handle;
					xQueueSend(I2C_commandQueue, &i2c_command,
							pdMS_TO_TICKS(5000));
				} else if (countSendStart_sens1 == 0
						&& readValSensor_dist1_final
								>= (Threshold_dist * speed_distance_sens_scaling)) {
					countSendStop_sens1 = 0;
					countSendStart_sens1++;
					i2c_command.command = I2C_START_MOTOR;
					i2c_command.commandValue = 0;
					i2c_command.sendingSensor = I2C_distance_sens_fw_dev_handle;
					xQueueSend(I2C_commandQueue, &i2c_command,
							pdMS_TO_TICKS(5000));
				}
				break;

			case I2C_distance_sens_bw_mux:
				uint16_t readValSensor2;
				readValSensor2 = VL53L0X_readRangeContinuousMillimeters(
						current_sensor->device_handle);
				readValSensor_dist2_final = readValSensor2;
				sendCommandApp(DistSensBw, (double*) &readValSensor_dist2_final,
						DOUBLE);
				static int countSendStop_sens2 = 0, countSendStart_sens2 = 0;
				if (readValSensor_dist2_final
						< (Threshold_dist * speed_distance_sens_scaling)
						&& countSendStop_sens2 == 0) {
					countSendStop_sens2++;
					countSendStart_sens2 = 0;
					i2c_command.command = I2C_STOP_MOTOR;
					i2c_command.commandValue = 0;
					i2c_command.sendingSensor = I2C_distance_sens_bw_handle;
					xQueueSend(I2C_commandQueue, &i2c_command, -1);
				} else if (countSendStart_sens2 == 0
						&& readValSensor_dist2_final
								>= (Threshold_dist * speed_distance_sens_scaling)) {
					countSendStop_sens2 = 0;
					countSendStart_sens2++;
					i2c_command.command = I2C_START_MOTOR;
					i2c_command.commandValue = 0;
					i2c_command.sendingSensor = I2C_distance_sens_bw_handle;
					xQueueSend(I2C_commandQueue, &i2c_command, -1);
				}
				break;

			case I2C_adxl345_sens_mux: // if
				double x, y, z;
				static double roll_init_offset = 0, pitch_init_offset = 0,
						count = 0;
				bool init_offset = false;
				I2C_read_adxl345_data(current_sensor->device_handle, &x, &y,
						&z);

				// Calculate roll and pitch
				double roll = atan2(y, sqrt(x * x + z * z)) * 180.0 / M_PI;
				double pitch = atan2(-x, sqrt(y * y + z * z)) * 180.0 / M_PI;
				static double roll_prev = 0, pitch_prev = 0, roll_final = 0,
						pitch_final = 0;
				if (init_offset == false) {
					count++;
					if (count < 6) {
						init_offset = true;
						roll_init_offset = roll;
						pitch_init_offset = pitch;
					}
				}
				roll -= roll_init_offset;
				pitch -= pitch_init_offset;

				/* Apply low pass filter */
				roll_final = ALPHA_ADXL * (double) roll
						+ (1 - ALPHA_ADXL) * (double) roll_prev;// Update the previous value for the next iteration
				roll_prev = roll;

				pitch_final = ALPHA_ADXL * (double) pitch
						+ (1 - ALPHA_ADXL) * (double) pitch_prev;// Update the previous value for the next iteration
				pitch_prev = pitch;

				sendCommandApp(ADXL_ROLL, (double*) &roll_final, DOUBLE);
				sendCommandApp(ADXL_PITCH, (double*) &pitch_final, DOUBLE);

				break;

			case I2C_pixy2_camera_mux:
#if (PIXY_DETECTION == ON)
				Vector goodVecLeft = { 0 };
				Vector goodVecRight = { 0 };
				pixy2Commands commandAutonomous;
				bool left = false;
				bool right = false;
				getPixy2Lines(I2C_pixy2_dev_handle, true, &lines); // Get all features (vectors, intersections, barcodes) with wait
				left = getBestVectorLeft(&lines, &goodVecLeft);
				right = getBestVectorRight(&lines, &goodVecRight);
				computeSpeedAndSteer(goodVecLeft, goodVecRight, left, right,
						&commandAutonomous);
				vTaskDelay(pdMS_TO_TICKS(15));
				if (AutonomousMode == true) {
					if (xQueueSend(autonomousModeControlPixyQueue,
							&commandAutonomous.computedSpeedSetpoint,
							portMAX_DELAY) != pdPASS) {
						ESP_LOGE("Error uart", "Unable to send to queue");
					}
					if (xQueueSend(autonomousModeControlPixyQueue,
							&commandAutonomous.computedSteerSetpoint,
							portMAX_DELAY) != pdPASS) {
						ESP_LOGE("Error uart", "Unable to send to queue");
					}
				}

#endif
				break;

			case I2C_temp_sens_mux:
				I2C_read_temperature(&temp);
				//ESP_LOGI("I2C", "Temp: [ %lf ]", temp);
				sendCommandApp(TEMPERATURE, (double*) &temp, DOUBLE);
				break;

			default:
				break;
			}
			current_sensor->state = SENSOR_READY;
			break;

		case SENSOR_READY:
			if (--current_sensor->tokens_remaining == 0) {
				current_sensor->tokens_remaining = current_sensor->tokens;
				current_sensor->state = SENSOR_IDLE;
				current_sensor_index = (current_sensor_index + 1)
						% sensor_count;
				current_sensor = &sensors[current_sensor_index];
			} else {
				current_sensor->state = SENSOR_READING;
			}
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}


bool I2C_addAndInitialiseSensors()
{
    config_rst_pin_i2c_mux();       // config rst pin for the mux
    rst_pin_i2c_mux_off();          // rst pin on
    vTaskDelay(pdMS_TO_TICKS(2000));
    rst_pin_i2c_mux_on();           // rst pin on

	//Add sensors
	I2C_add_device(I2C_mux_addr);// add the multiplexer to the I2C bus
	I2C_add_device(I2C_temp_sens_addr);
	I2C_add_device(I2C_distance_sens_addr);
	I2C_add_device(I2C_adxl345_sens_addr);
	I2C_add_device(I2C_pixy2_camera_addr);

	//Check Pixy2 camera version


	//Init sensors
	I2C_adxl345_init(I2C_adxl345_sens_dev_handle);
	I2C_adxl345_setRate(I2C_adxl345_sens_dev_handle, ADXL1345_READ_RATE);
	I2C_temperatureSensor_trigger_measurement();
	bool dist_sens_init = VL53L0X_Init(I2C_distance_sens_bw_handle);
	while(dist_sens_init == false) {vTaskDelay(pdMS_TO_TICKS(50));}
	dist_sens_init = false;
	dist_sens_init = VL53L0X_Init(I2C_distance_sens_fw_dev_handle);
	while(dist_sens_init == false) {vTaskDelay(pdMS_TO_TICKS(50));}
	requestPixy2Version(I2C_pixy2_dev_handle);
	return true;
}

void start_InitI2c_and_I2C_devices_task()
{
	xTaskCreatePinnedToCore(I2C_devices_task, "I2C_devices_task", 4096, NULL, 7, NULL, 0);
}




