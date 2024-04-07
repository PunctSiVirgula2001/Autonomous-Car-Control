#include "driver/i2c_master.h"

// I2C config
#define I2C_SDA 21
#define I2C_SCL 22
#define ALLOW_INTERNAL_PULLUPS true
#define GLITCH_IGNORE_CNT 7
#define MULTIPLEXER_TCA9548A_ADDRESS 0x70

// I2C 8 devices connected to the I2C multiplexer
enum I2C_devices_multiplexer {
    I2C_temp_sens =       0x01,
    I2C_distance_sens_1 = 0x02,
    I2C_distance_sens_2 = 0x03,
    I2C_pixy2_camera =    0x04,
    I2C_device5 = 0x05,         // not used yet
    I2C_device6 = 0x06,
    I2C_device7 = 0x07,
    I2C_device8 = 0x08,
};



// Init I2C for esp32 as master
void I2C_master_init();

// Task for I2C devices
void I2C_devices_task(void *pvParameters);

