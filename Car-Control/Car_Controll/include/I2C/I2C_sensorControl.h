/*
 * I2C_sensorControl.c
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */
#ifndef I2C_SENSOR_CONTROL_H
#define I2C_SENSOR_CONTROL_H


#include "drivers/I2C_accelerometerSensor.h"
#include "drivers/I2C_cameraSensor.h"
#include "drivers/I2C_distanceSensor.h"
#include "drivers/I2C_temperatureSensor.h"

typedef enum I2C_WRR_tokens
{
	pixy2 = 1,
	distance_sens1 = 0,
	distance_sens2 = 0,
	adxl_acc = 0,
	temp_sens = 1
}I2C_WRR_tokens;

typedef enum {
    SENSOR_IDLE,
    SENSOR_READING,
    SENSOR_READY
} sensor_state_t;

typedef enum
{
  I2C_NO_STATE,
  I2C_STOP_MOTOR,
  I2C_START_MOTOR,
  I2C_NEW_STEER,
  I2C_MOTOR_OFFSET_ACCELEROMETER
}I2C_COMMAND_TYPE;

typedef struct {
    I2C_dev_handles device_handle;
    I2C_devices_mux mux;
    I2C_WRR_tokens tokens;
    I2C_WRR_tokens tokens_remaining;
    sensor_state_t state;
} sensor_t;

typedef struct I2C_COMMAND{
  I2C_dev_handles sendingSensor;
  I2C_COMMAND_TYPE command;
  uint16_t commandValue;
}I2C_COMMAND;

void start_InitI2c_and_I2C_devices_task();
bool I2C_addAndInitialiseSensors();
void I2C_devices_task(void *pvParameters);

#endif // I2C_SENSOR_CONTROL_H
