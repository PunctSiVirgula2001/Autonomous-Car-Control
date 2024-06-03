# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS "main.c" "../src/MotorControl_mcpwm.c" "../src/MotorAndServoControl.c" "../src/PID.c" "../src/Network.c" "../src/I2C_devices.c" "../src/UART_Jetson.c"
    INCLUDE_DIRS "../include"
    PRIV_INCLUDE_DIRS  # optional, add here private include directories
    REQUIRES driver esp_wifi nvs_flash esp_timer app_trace
    PRIV_REQUIRES
)
