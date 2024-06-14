# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.


# Define the component
idf_component_register(
    SRCS "../src/I2C/drivers/I2C_accelerometerSensor.c"
         "../src/I2C/drivers/I2C_cameraSensor.c"
         "../src/I2C/drivers/I2C_common.c"
         "../src/I2C/drivers/I2C_distanceSensor.c"
         "../src/I2C/drivers/I2C_temperatureSensor.c"
         "../src/I2C/I2C_sensorControl.c"
         "../src/MotorAndServoControl.c"
         "../src/Network.c"
         "../src/PID.c"
         "../src/UART_Jetson.c"
         "main.c"
    INCLUDE_DIRS "../include"
                 "../include/I2C"
                 "../include/I2C/drivers"
    REQUIRES driver esp_wifi nvs_flash esp_timer
)