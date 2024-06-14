/*
 * I2C_sensorControl.c
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#include "I2C_sensorControl.h"

QueueHandle_t I2C_commandQueue;
bool I2C_sensors_initiated = false;
I2C_COMMAND i2c_command;
int speed_distance_sens_scaling;


void I2C_devices_task(void *pvParameters) {
	double temp;
    I2C_sensors_initiated = false;

    // Initialize ESP32 as master.
    I2C_master_init();

    ESP_LOGI("I2C_sensors", "Sensors init start!");
    // Initialize sensors.
    I2C_sensors_initiated = I2C_addAndInitialiseSensors();
    if(I2C_sensors_initiated == false)
    	ESP_LOGI("I2C_sensors", "Sensors init failed!");
    else
    	ESP_LOGI("I2C_sensors", "Sensors successfully initiated!");

    // Create structure for managing each sensor.
    sensor_t sensors[] = {
        {I2C_distance_sens1_dev_handle, I2C_distance_sens_1_mux, distance_sens1, distance_sens1, SENSOR_IDLE},
        {I2C_distance_sens2_dev_handle, I2C_distance_sens_2_mux, distance_sens2, distance_sens2, SENSOR_IDLE},
        {I2C_adxl345_sens_dev_handle, I2C_adxl345_sens_mux, adxl_acc, adxl_acc, SENSOR_IDLE},
        {I2C_pixy2_dev_handle, I2C_pixy2_camera_mux, pixy2, pixy2, SENSOR_IDLE},
        {I2C_temp_sens_dev_handle, I2C_temp_sens_mux, temp_sens, temp_sens, SENSOR_IDLE}
    };

    I2C_COMMAND i2c_command = {0, 0, 0};

    size_t sensor_count = sizeof(sensors) / sizeof(sensor_t);
    size_t current_sensor_index = 0;
    sensor_t *current_sensor = &sensors[current_sensor_index];

    while (1) {
        // Skip sensors with 0 tokens
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
                    case I2C_distance_sens_1_mux:
                    	uint16_t readValSensor1;
                    	// Read new sensor value
						readValSensor1 = VL53L0X_readRangeContinuousMillimeters(I2C_distance_sens1_dev_handle);
						// Apply low-pass filter
						readValSensor_dist1_final = readValSensor1;
						//readValSensor_dist1_final = ALPHA_VL53L0X * (double)readValSensor1 + (1 - ALPHA_VL53L0X) * (double)readValSensor_dist1_prev;
						// Update the previous value for the next iteration
						//readValSensor_dist1_prev = readValSensor1;
						// Make a decision

						sendCommandApp(DistSensFw, (double*)&readValSensor_dist1_final, DOUBLE);
						static int countSendStop_sens1 = 0,countSendStart_sens1 = 0;
						if(readValSensor_dist1_final < (Threshold_dist * speed_distance_sens_scaling) && countSendStop_sens1 == 0)
						{
							countSendStop_sens1++;
							countSendStart_sens1=0;
							i2c_command.command = I2C_STOP_MOTOR;
							i2c_command.commandValue = 0;
							i2c_command.sendingSensor = I2C_distance_sens1_dev_handle;
							xQueueSend(I2C_commandQueue,&i2c_command,pdMS_TO_TICKS(5000));
							ESP_LOGI("", "Sent STOP for forward \n");
						}
						else if (countSendStart_sens1 == 0 && readValSensor_dist1_final >= (Threshold_dist * speed_distance_sens_scaling))
						{
							countSendStop_sens1=0;
							countSendStart_sens1++;
							i2c_command.command = I2C_START_MOTOR;
							i2c_command.commandValue = 0;
							i2c_command.sendingSensor = I2C_distance_sens1_dev_handle;
							xQueueSend(I2C_commandQueue,&i2c_command,pdMS_TO_TICKS(5000));
							ESP_LOGI("", "Sent START for forward \n");
						}

						// Log the filtered value
						ESP_LOGI("I2C", "Range1: [ %.2lf ]", readValSensor_dist1_final);
                        //vTaskDelay(pdMS_TO_TICKS(50)); // Allow for multiplexer switch
                        break;

                    case I2C_distance_sens_2_mux:
                    	uint16_t readValSensor2;
                        readValSensor2 = VL53L0X_readRangeContinuousMillimeters(current_sensor->device_handle);
//                        readValSensor_dist2_final = ALPHA_VL53L0X * (double)readValSensor2 + (1 - ALPHA_VL53L0X) * (double)readValSensor_dist2_prev;						// Update the previous value for the next iteration
//                        readValSensor_dist2_prev = readValSensor2;
                        readValSensor_dist2_final = readValSensor2;
                        sendCommandApp(DistSensBw, (double*)&readValSensor_dist2_final, DOUBLE);
                        static int countSendStop_sens2 = 0,countSendStart_sens2 = 0;
                        if(readValSensor_dist2_final < (Threshold_dist * speed_distance_sens_scaling) && countSendStop_sens2 == 0)
						{
                        	countSendStop_sens2++;
                        	countSendStart_sens2=0;
							i2c_command.command = I2C_STOP_MOTOR;
							i2c_command.commandValue = 0;
							i2c_command.sendingSensor = I2C_distance_sens2_dev_handle;
							xQueueSend(I2C_commandQueue,&i2c_command,-1);
							ESP_LOGI("", "Sent STOP for backward \n");
						}
                        else if (countSendStart_sens2 == 0 && readValSensor_dist2_final >= (Threshold_dist * speed_distance_sens_scaling))
                        {
                        	countSendStop_sens2=0;
                        	countSendStart_sens2++;
                        	i2c_command.command = I2C_START_MOTOR;
                        	i2c_command.commandValue = 0;
                        	i2c_command.sendingSensor = I2C_distance_sens2_dev_handle;
                        	xQueueSend(I2C_commandQueue,&i2c_command,-1);
                        }
                          // Log the filtered value
                        ESP_LOGI("I2C", "Range2: [ %.2lf ]", readValSensor_dist2_final);
                        break;

                    case I2C_adxl345_sens_mux: // if
						 double x, y, z;
						 static double roll_init_offset = 0, pitch_init_offset = 0, count = 0;
						 bool init_offset = false;
						I2C_read_adxl345_data(current_sensor->device_handle, &x, &y, &z);

						// Calculate roll and pitch
						double roll = atan2(y, sqrt(x * x + z * z)) * 180.0 / M_PI;
						double pitch = atan2(-x, sqrt(y * y + z * z)) * 180.0 / M_PI;
						static double roll_prev=0, pitch_prev=0, roll_final = 0, pitch_final = 0;
						if(init_offset == false)
						{
							count++;
							if(count<6)
							{
								init_offset = true;
								roll_init_offset = roll;
								pitch_init_offset = pitch;
							}
						}
						roll -=roll_init_offset;
						pitch -=pitch_init_offset;

						/* Apply low pass filter */
						roll_final = ALPHA_ADXL * (double)roll + (1 - ALPHA_ADXL) * (double)roll_prev;						// Update the previous value for the next iteration
						roll_prev = roll;

						pitch_final = ALPHA_ADXL * (double)pitch + (1 - ALPHA_ADXL) * (double)pitch_prev;						// Update the previous value for the next iteration
						pitch_prev = pitch;

						ESP_LOGI("I2C", "Orientation roll=%lf pitch=%lf ", roll_final, pitch_final);

						sendCommandApp(ADXL_ROLL, (double*)&roll_final, DOUBLE);
						sendCommandApp(ADXL_PITCH, (double*)&pitch_final, DOUBLE);

						break;

                    case I2C_pixy2_camera_mux:
                        // Implement camera read logic here
                        break;

                    case I2C_temp_sens_mux:
                        I2C_read_temperature(&temp);
                        ESP_LOGI("I2C", "Temp: [ %lf ]", temp);
                        sendCommandApp(TEMPERATURE, (double*)&temp, DOUBLE);
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
                    current_sensor_index = (current_sensor_index + 1) % sensor_count;
                    current_sensor = &sensors[current_sensor_index];
                } else {
                    current_sensor->state = SENSOR_IDLE;
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
    vTaskDelay(pdMS_TO_TICKS(500));
    rst_pin_i2c_mux_on();           // rst pin on

	//Add sensors
	I2C_add_device(I2C_mux_addr);// add the multiplexer to the I2C bus
	I2C_add_device(I2C_temp_sens_addr);
	I2C_add_device(I2C_distance_sens_addr);
	I2C_add_device(I2C_adxl345_sens_addr);

	//Init sensors
	I2C_adxl345_init(I2C_adxl345_sens_dev_handle);
	I2C_adxl345_setRate(I2C_adxl345_sens_dev_handle, ADXL1345_READ_RATE);
	I2C_temperatureSensor_trigger_measurement();
	bool dist_sens_init = VL53L0X_Init(I2C_distance_sens2_dev_handle);
	while(dist_sens_init == false) {vTaskDelay(pdMS_TO_TICKS(50));}
	dist_sens_init = false;
	dist_sens_init = VL53L0X_Init(I2C_distance_sens1_dev_handle);
	while(dist_sens_init == false) {vTaskDelay(pdMS_TO_TICKS(50));}

	return true;
}

void start_I2C_devices_task()
{
	xTaskCreatePinnedToCore(I2C_devices_task, "I2C_devices_task", 4096, NULL, 7, NULL, 0);
}

