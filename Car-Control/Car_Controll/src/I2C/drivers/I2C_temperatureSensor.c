/*
 * I2C_temperatureSensor.c
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

/******** BMP/BME280 sensor **********/
//I2C functions for accessing the BMP/BME280 sensor : temperature, pressure, humidity

#include "I2C_temperatureSensor.h"


struct bme280_calib_data temp_calibration;

// I2C temperature sensor BMP functions
void I2C_temperatureSensor_trigger_measurement(){
	uint8_t calib[24];
	I2C_transmit(I2C_temp_sens_dev_handle, (unsigned char[]){0x88},1);
	vTaskDelay(pdMS_TO_TICKS(10));
	I2C_receive(I2C_temp_sens_dev_handle, calib,24);
	vTaskDelay(pdMS_TO_TICKS(10));
	// Convert the bytes into calibration data
	temp_calibration.dig_T1 = (uint16_t)(calib[1] << 8) | calib[0];
	temp_calibration.dig_T2 = (int16_t)(calib[3] << 8) | calib[2];
	temp_calibration.dig_T3 = (int16_t)(calib[5] << 8) | calib[4];

    // Define the control register address for BMP280/BME280
    uint8_t ctrl_meas_reg = 0xF4;
    // Write a value to trigger a measurement: use 0x2X for force mode, 0x3X for normal mode
    // The 'X' should be replaced with the combination of temperature and pressure oversampling
    // Example: 0x27 for normal mode, oversampling x1 for both temperature and pressure
    uint8_t ctrl_meas_value = 0x27;
    // Transmit the address and the value
    I2C_transmit(I2C_temp_sens_dev_handle, (unsigned char[]){ctrl_meas_reg, ctrl_meas_value},2);
}

void I2C_read_temperature(double *fine_temp)
{
	// Read temperature registers
	uint8_t temp_msb;
	uint8_t temp_lsb;
	uint8_t temp_xlsb;
	uint32_t raw_temp;
	I2C_transmit(I2C_temp_sens_dev_handle, (unsigned char[]){0xFA},1);
	I2C_receive(I2C_temp_sens_dev_handle, &temp_msb,8);

	I2C_transmit(I2C_temp_sens_dev_handle, (unsigned char[]){0xFB},1);
	I2C_receive(I2C_temp_sens_dev_handle, &temp_lsb,8);

	I2C_transmit(I2C_temp_sens_dev_handle, (unsigned char[]){0xFC},1);
	I2C_receive(I2C_temp_sens_dev_handle, &temp_xlsb,4);
	// Combine bytes to get the raw temperature value
	raw_temp = ((uint32_t)temp_msb << 12) | ((uint32_t)temp_lsb << 4) | (temp_xlsb >> 4);

	double var1, var2;
	var1  = (((double)raw_temp)/16384.0-((double)temp_calibration.dig_T1)/1024.0) * ((double)temp_calibration.dig_T2);
	var2  = ((((double)raw_temp)/131072.0-((double)temp_calibration.dig_T1)/8192.0) *(((double)raw_temp)/131072.0-((double) temp_calibration.dig_T1)/8192.0)) * ((double)temp_calibration.dig_T3);
	*fine_temp  = (var1 + var2) / 5120.0;
}

// I2c pressure sensor BME functions
void I2C_set_pressure_register()
{
   I2C_transmit(I2C_temp_sens_dev_handle, (unsigned char[]){0xF7},1); // Pressure register address
}

// I2c humidity sensor BME/BMP functions
void I2C_set_humidity_register(){
   I2C_transmit(I2C_temp_sens_dev_handle, (unsigned char[]){0xFD},1); // Humidity register address
}
