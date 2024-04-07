#include "I2C_devices.h"
//i2c config structure
i2c_master_bus_config_t esp32_i2c_config;
//i2c bus handle for the configuration
i2c_master_bus_handle_t bus_handle_esp32_i2c_config;
//i2c device handle for the configuration
i2c_master_dev_handle_t device_handle_esp32_i2c_config;

// initialize I2C for esp32 as master
void I2C_master_init()
{
	esp32_i2c_config.i2c_port = -1;											// automatically select the next available i2c port
    esp32_i2c_config.sda_io_num = I2C_SDA;
    esp32_i2c_config.scl_io_num = I2C_SCL;
    esp32_i2c_config.clk_source = I2C_CLK_SRC_DEFAULT;
	esp32_i2c_config.flags.enable_internal_pullup = ALLOW_INTERNAL_PULLUPS; // might need external pullups as well
	esp32_i2c_config.trans_queue_depth = 4; 								// queue size when the communication is asynchronous
	esp32_i2c_config.glitch_ignore_cnt = GLITCH_IGNORE_CNT; 				// number of clock cycles to ignore for glitch filtering
    ESP_ERROR_CHECK(i2c_new_master_bus(&esp32_i2c_config, &bus_handle_esp32_i2c_config));
}

// add device to the I2C bus with different address and speed
void I2C_add_device(uint8_t device_address, i2c_master_bus_handle_t bus_handle, uint32_t scl_speed, i2c_master_dev_handle_t device_handle)
{
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = scl_speed
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &device_config, &device_handle));
}
// add multiplexer to the I2C bus by using I2C_add_device
void I2C_add_multiplexer(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t device_handle)
{
    I2C_add_device(MULTIPLEXER_TCA9548A_ADDRESS, bus_handle, 100000, device_handle);
}

// switch the multiplexer channel



// Task for I2C devices
void I2C_devices_task(void *pvParameters)
{
    I2C_master_init();
    // add devices to the I2C bus
}
