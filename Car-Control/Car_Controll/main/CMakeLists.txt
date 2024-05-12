# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c MotorControl_mcpwm.c MotorAndServoControl.c PID.c Network.c I2C_devices.c UART_Jetson.c  # list the source files of this component
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            driver esp_wifi nvs_flash esp_timer # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
