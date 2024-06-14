#ifndef I2C_COMMON_H
#define I2C_COMMON_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_task_wdt.h"
#include "driver/i2c_master.h"
#include "math.h"
#include "Network.h"
#include <stdbool.h>

// I2C config
#define I2C_SDA 21
#define I2C_SCL 22
#define INTERNAL_PULLUPS false
#define GLITCH_IGNORE_CNT 7
#define SCL_SPEED_FAST_MODE 400000
#define SCL_SPEED_SLOW_MODE 100000
#define RST_PIN_MUX_I2C 23
#define I2C_COMMAND_SIZE (sizeof(I2C_COMMAND))
#define QUEUE_SIZE_I2C 10

// I2C 8 devices connected to the I2C multiplexer. --> control register mux
typedef enum I2C_devices_multiplexer {
	NO_MUX_SELECTED = -1,
    I2C_distance_sens_1_mux  = 0,
    I2C_distance_sens_2_mux  = 1,
	I2C_adxl345_sens_mux 	 = 3,
	I2C_temp_sens_mux		 = 2,
	I2C_oled_display_096_mux = 4,
    I2C_pixy2_camera_mux 	 = 5,
}I2C_devices_mux;

// Each device connected to the car that works with I2C. --> device address
typedef enum I2C_devices{
	I2C_mux_addr			  = 0x70, // 1110 000 0/1 transmit/receive
	I2C_temp_sens_addr		  = 0x76,
	I2C_distance_sens_addr    = 0x29,
	I2C_pixy2_camera_addr 	  = 0x54,
	I2C_adxl345_sens_addr 	  = 0x53,
	I2C_oled_display_096_addr = 0x00
}I2C_devices;

typedef enum I2C_dev_handles
{
  I2C_multiplexer_dev_handle,
  I2C_pixy2_dev_handle,
  I2C_distance_sens1_dev_handle, // fata
  I2C_distance_sens2_dev_handle, // spate
  I2C_temp_sens_dev_handle,
  I2C_oled_display_096_dev_handle,
  I2C_adxl345_sens_dev_handle,
  I2C_MAX_Num_of_dev_handles
}I2C_dev_handles;


// Init I2C for esp32 as master
void I2C_master_init();
void I2C_select_multiplexer_channel(I2C_devices_mux num_channel);
void I2C_transmit(I2C_dev_handles device_handle, unsigned char* data, size_t write_size);
void I2C_receive(I2C_dev_handles device_handle, uint8_t* data, size_t read_size);
uint8_t I2C_readRegister8bit(I2C_dev_handles device_handle,uint8_t reg);
uint16_t I2C_readRegister16bit(I2C_dev_handles device_handle, uint8_t reg);
uint32_t I2C_readRegister32bit(I2C_dev_handles device_handle, uint8_t reg);
void I2C_writeRegister8bit(I2C_dev_handles device_handle, uint8_t reg, uint8_t value);
void I2C_writeRegister16bit(I2C_dev_handles device_handle, uint8_t reg, uint16_t value);
void I2C_writeRegister32bit(I2C_dev_handles device_handle, uint8_t reg, uint32_t value);
void writeMultipleRegisters(I2C_dev_handles device_handle, const uint8_t (*regs_vals)[2], size_t count);
void I2C_add_device(uint8_t device_address);
void config_rst_pin_i2c_mux();
void rst_pin_i2c_mux_on();
void rst_pin_i2c_mux_off();



#endif // I2C_COMMON_H


