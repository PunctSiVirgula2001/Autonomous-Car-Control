/*
 * I2C_accelerometerSensor.h
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#ifndef INCLUDE_I2C_I2C_ACCELEROMETERSENSOR_H_
#define INCLUDE_I2C_I2C_ACCELEROMETERSENSOR_H_

#include "I2C_common.h"

/* ADXL 345 accelerometer sensor */
#define ADXL1345_READ_RATE 200.0 // HZ
#define ALPHA 0.8

typedef enum ADXL_345_reg
{
  ADXL345_REG_BW_RATE 	  = 0x2C,
  ADXL345_REG_POWER_CTL   = 0x2D,
  ADXL345_REG_DATA_FORMAT = 0x31,
  ADXL345_REG_DATAX0      = 0x32,
  ADXL345_REG_DATAX1	  = 0x33,
  ADXL345_REG_DATAY0	  = 0x34,
  ADXL345_REG_DATAY1	  = 0x35,
  ADXL345_REG_DATAZ0	  = 0x36,
  ADXL345_REG_DATAZ1	  = 0x37
}ADX_345_reg;

void I2C_read_adxl345_data(I2C_dev_handles device_handle, double* x, double* y, double* z);
void I2C_adxl345_init(I2C_dev_handles device_handle);
void I2C_adxl345_setRate(I2C_dev_handles device_handle, double rate);

#endif /* INCLUDE_I2C_I2C_ACCELEROMETERSENSOR_H_ */
