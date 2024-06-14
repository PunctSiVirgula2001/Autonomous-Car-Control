/*
 * I2C_accelerometer.c
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#include "I2C_accelerometerSensor.h"


/* ADXL 345 accelerometer sensor */

void I2C_adxl345_init(I2C_dev_handles device_handle) {
    I2C_writeRegister8bit(device_handle, ADXL345_REG_POWER_CTL, 0x08);  // Bit 3 high to start measuring
    I2C_writeRegister8bit(device_handle, ADXL345_REG_DATA_FORMAT, 0x0B); // Plus/minus 16g, 13-bit mode
    I2C_writeRegister8bit(device_handle, ADXL345_REG_BW_RATE, 0x0B); // Set data rate to 100 Hz (example)
}

void I2C_read_adxl345_data(I2C_dev_handles device_handle, double* x, double* y, double* z) {
    uint8_t data[6];  // Array to hold the raw sensor data
    // Update previous values
	static double prev_x = 0;
	static double prev_y = 0;
	static double prev_z = 0;
    double resolution = 1<<8;
    // Read 6 bytes from the ADXL345 starting from DATAX0
    uint16_t temp;
    temp = I2C_readRegister16bit(device_handle, ADXL345_REG_DATAX0);
    data[0] =(uint8_t)(temp>>8);
    data[1] =(uint8_t)temp;
    temp = I2C_readRegister16bit(device_handle, ADXL345_REG_DATAY0);
    data[2] =(uint8_t)(temp>>8);
    data[3] =(uint8_t)temp;
    temp = I2C_readRegister16bit(device_handle, ADXL345_REG_DATAZ0);
    data[4] =(uint8_t)(temp>>8);
    data[5] =(uint8_t)temp;

    // Combine the bytes into integers
    double raw_x  = (double)((int16_t)(data[1] << 8 | data[0]))/resolution;
    double raw_y  = (double)((int16_t)(data[3] << 8 | data[2]))/resolution;
    double raw_z  = (double)((int16_t)(data[5] << 8 | data[4]))/resolution;

    // Apply low-pass filter
   *x = ALPHA * raw_x + (1 - ALPHA) * prev_x;
   *y = ALPHA * raw_y + (1 - ALPHA) * prev_y;
   *z = ALPHA * raw_z + (1 - ALPHA) * prev_z;

   // Update previous values
   prev_x = *x;
   prev_y = *y;
   prev_z = *z;
}

void I2C_adxl345_setRate(I2C_dev_handles device_handle, double rate) {
    uint8_t _b, _s; // Variables to hold register values
    int v = (int) (rate / 6.25); // Convert the desired rate to a value used by the sensor
    int r = 0; // Variable to determine the bit position for the rate setting

    // Calculate the rate setting value based on the input rate
    // This loop determines how many times the value can be shifted right
    // before it becomes zero, effectively calculating the position of the highest set bit.
    while (v >>= 1) {
        r++;
    }

    // The valid range for r is 0 to 9, corresponding to available data rate settings
    if (r <= 9) {
        // Read the current value of the BW_RATE register
        // The BW_RATE register controls the data rate and power mode of the sensor
    	_b = (uint8_t)(I2C_readRegister16bit(device_handle, ADXL345_REG_BW_RATE)>>8);

        // Calculate the new value for the BW_RATE register
        // The lower 4 bits of the BW_RATE register (D3-D0) determine the output data rate
        // We preserve the upper 4 bits (D7-D4) by masking with 0xF0 and OR with the new rate setting
        _s = (uint8_t) (r + 6) | (_b & 0xF0);

        // Write the new value to the BW_RATE register
        I2C_writeRegister8bit(device_handle, ADXL345_REG_BW_RATE, _s);
    }
}
