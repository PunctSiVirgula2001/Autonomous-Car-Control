/*
 * I2C_temperatureSensor.h
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#ifndef INCLUDE_I2C_I2C_TEMPERATURESENSOR_H_
#define INCLUDE_I2C_I2C_TEMPERATURESENSOR_H_

#include "I2C_common.h"

/******** BMP/BME280 sensor **********/
struct bme280_calib_data {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    // Add humidity calibration data here if needed
};
// I2C temperature sensor BMP functions
void I2C_temperatureSensor_trigger_measurement();
// I2C pressure sensor BME functions
void I2C_set_pressure_register();
// I2C humidity sensor BME/BMP functions
void I2C_set_humidity_register();
void I2C_read_temperature(double *fine_temp);
// Task for I2C devices
void I2C_devices_task(void *pvParameters);


#endif /* INCLUDE_I2C_I2C_TEMPERATURESENSOR_H_ */
