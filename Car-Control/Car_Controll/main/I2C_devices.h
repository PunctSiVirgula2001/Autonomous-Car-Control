#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// I2C config
#define I2C_SDA 21
#define I2C_SCL 22
#define INTERNAL_PULLUPS false
#define GLITCH_IGNORE_CNT 7
#define SCL_SPEED_FAST_MODE 400000
#define SCL_SPEED_LOW_MODE 100000
#define RST_PIN_MUX_I2C 23

// I2C 8 devices connected to the I2C multiplexer. --> control register mux
typedef enum I2C_devices_multiplexer {
    I2C_distance_sens_1_mux  = 0,
    I2C_distance_sens_2_mux  = 1,
	I2C_temp_sens_mux		 = 2,
	I2C_oled_display_096_mux = 3,
    I2C_pixy2_camera_mux 	 = 4,
}I2C_devices_mux;

// Each device connected to the car that works with I2C. --> device address
typedef enum I2C_devices{
	I2C_mux_addr			  = 0x70, // 1110 000 0/1 transmit/receive
	I2C_temp_sens_addr		  = 0x76,
	I2C_distance_sens_addr    = 0x29,
	I2C_pixy2_camera_addr 	  = 0x54,
	I2C_oled_display_096_addr = 0x00
}I2C_devices;

typedef enum I2C_dev_handles
{
  I2C_multiplexer_dev_handle,
  I2C_pixy2_dev_handle,
  I2C_distance_sens1_dev_handle,
  I2C_distance_sens2_dev_handle,
  I2C_temp_sens_dev_handle,
  I2C_oled_display_096_dev_handle,
  I2C_MAX_Num_of_dev_handles
}I2C_dev_handles;


// Init I2C for esp32 as master
void I2C_master_init();

void I2C_select_multiplexer_channel(I2C_devices_mux num_channel);

void I2C_transmit(I2C_dev_handles device_handle, unsigned char* data);

void I2C_receive(I2C_dev_handles device_handle, uint8_t* data);

void I2C_probe(I2C_devices device_addr);

void I2C_add_device(uint8_t device_address);

void start_I2C_devices_task();

void config_rst_pin_i2c_mux();

void rst_pin_i2c_mux_on();

void rst_pin_i2c_mux_off();

/******** BMP/BME280 sensor **********/
// I2C temperature sensor BMP functions
void I2C_read_temperature(uint32_t *raw_temp);
void I2C_trigger_measurement();
// I2C pressure sensor BME functions
void I2C_set_pressure_register();
// I2C humidity sensor BME/BMP functions
void I2C_set_humidity_register();

// Task for I2C devices
void I2C_devices_task(void *pvParameters);

