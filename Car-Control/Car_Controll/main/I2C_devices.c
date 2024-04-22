#include "I2C_devices.h"
#include "Network.h"

//i2c config structure
i2c_master_bus_config_t esp32_i2c_config;
//i2c bus handle for the configuration
i2c_master_bus_handle_t bus_handle_esp32_i2c_config;
//i2c device handle for the configuration
i2c_master_dev_handle_t device_handle_esp32_i2c_config[I2C_MAX_Num_of_dev_handles];

// initialize I2C for esp32 as master
void I2C_master_init()
{
	esp32_i2c_config.i2c_port = I2C_NUM_0;											// automatically select the next available i2c port
    esp32_i2c_config.sda_io_num = I2C_SDA;
    esp32_i2c_config.scl_io_num = I2C_SCL;
    esp32_i2c_config.clk_source = I2C_CLK_SRC_DEFAULT;
	esp32_i2c_config.flags.enable_internal_pullup = INTERNAL_PULLUPS; // might need external pullups as well
	esp32_i2c_config.trans_queue_depth = 4; 								// queue size when the communication is asynchronous
	esp32_i2c_config.glitch_ignore_cnt = GLITCH_IGNORE_CNT; 				// number of clock cycles to ignore for glitch filtering
	esp32_i2c_config.intr_priority = 0;
    ESP_ERROR_CHECK(i2c_new_master_bus(&esp32_i2c_config, &bus_handle_esp32_i2c_config));
}

// switch the multiplexer channel
void I2C_select_multiplexer_channel(I2C_devices_mux num_channel)
{
	// Prepare the data to send: one byte where each bit represents a channel
	uint8_t data = 1 << num_channel; // Shift 1 to the correct bit position for the channel
	// Perform the I2C write to change the channel
	esp_err_t ret = i2c_master_transmit(device_handle_esp32_i2c_config[I2C_multiplexer_dev_handle], &data, 1, -1);
	// Check the result
	if(ret==ESP_OK)
	ESP_LOGI("I2C", "Channel %d selected", num_channel);
	// Check the result
	if (ret != ESP_OK)
	{
		ESP_LOGI("I2C ERR", "ERROR Channel %d not selected", num_channel);
	}
}

// add device to the I2C bus with different address and speed
void I2C_add_device(uint8_t device_address)
{
	static unsigned char mux_added = 0U;
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = SCL_SPEED_FAST_MODE
    };

    switch (device_address) {
      case I2C_temp_sens_addr:
    	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_temp_sens_mux);
    	  vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_temp_sens_dev_handle]));
    	  }
    	  break;
      case I2C_distance_sens_addr:
    	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_distance_sens_1_mux);
    	  vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_distance_sens1_dev_handle])); // distance sensors have the same addr
    	  vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
    	  I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);
    	  vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_distance_sens2_dev_handle])); // distance sensors have the same addr
    	  }
    	  break;
      case I2C_pixy2_camera_addr:
      	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
    	  vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_pixy2_dev_handle]));
    	  }
    	  break;
      case I2C_oled_display_096_addr:
    	  if(mux_added){
          I2C_select_multiplexer_channel(I2C_oled_display_096_mux);
          vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_oled_display_096_dev_handle]));
    	  }
    	  break;
      case I2C_mux_addr:
    	  mux_added = 1U;
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_multiplexer_dev_handle]));
    	break;
      default:
    }
}


void I2C_transmit(I2C_dev_handles device_handle, unsigned char* data, size_t write_size)
{
	static I2C_dev_handles old_handle=99;
	static I2C_dev_handles current_handle;

	current_handle=device_handle;
	if(old_handle!=current_handle){
	//Before transmitting choose the correct channel for the mux
	switch (device_handle) {
		case I2C_multiplexer_dev_handle:
			// do nothing
		break;
		case I2C_pixy2_dev_handle:
			 I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
			 old_handle=I2C_pixy2_dev_handle;
		break;
		case I2C_distance_sens1_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_1_mux);
			old_handle=I2C_distance_sens1_dev_handle;
		break;
		case I2C_distance_sens2_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);
			old_handle=I2C_distance_sens2_dev_handle;
		break;
		case I2C_temp_sens_dev_handle:
			I2C_select_multiplexer_channel(I2C_temp_sens_mux);
			old_handle=I2C_temp_sens_dev_handle;
		break;
		case I2C_oled_display_096_dev_handle:
			I2C_select_multiplexer_channel(I2C_oled_display_096_mux);
			old_handle=I2C_oled_display_096_dev_handle;
		break;
		default:
	 }
	}
	vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
	ESP_ERROR_CHECK(i2c_master_transmit(device_handle_esp32_i2c_config[device_handle], (unsigned char*)data, write_size, -1)); // 3rd argument = length of the data in bytes ==> only commands, 2 bytes only
}

void I2C_receive(I2C_dev_handles device_handle, uint8_t* data, size_t read_size)
{
	static I2C_dev_handles old_handle=99;
	static I2C_dev_handles current_handle;

	current_handle=device_handle;
	if(old_handle!=current_handle)
	switch (device_handle) {
		case I2C_multiplexer_dev_handle:
			// do nothing
		break;
		case I2C_pixy2_dev_handle:
			 I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
			 old_handle=I2C_pixy2_dev_handle;
		break;
		case I2C_distance_sens1_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_1_mux);
			old_handle=I2C_distance_sens1_dev_handle;
		break;
		case I2C_distance_sens2_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);
			old_handle=I2C_distance_sens2_dev_handle;
		break;
		case I2C_temp_sens_dev_handle:
			I2C_select_multiplexer_channel(I2C_temp_sens_mux);
			old_handle=I2C_temp_sens_dev_handle;
		break;
		case I2C_oled_display_096_dev_handle:
			I2C_select_multiplexer_channel(I2C_oled_display_096_mux);
			old_handle=I2C_oled_display_096_dev_handle;
		break;
		default:
	}
	vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
	ESP_ERROR_CHECK(i2c_master_receive(device_handle_esp32_i2c_config[device_handle], (uint8_t*)data, read_size, -1)); // 3rd argument = length of the data in bytes ==> only commands, 2 bytes only
}

void I2C_probe(I2C_devices device_addr)
{
	i2c_master_probe(bus_handle_esp32_i2c_config,device_addr,1000); // 1000ms timeout
}

// Task for I2C devices
void I2C_devices_task(void *pvParameters)
{
	/* Initial config I2C */
	config_rst_pin_i2c_mux();	 //config rst pin for the mux
	rst_pin_i2c_mux_on();		 //rst pin on
    I2C_master_init();			 //init master
    I2C_add_device(I2C_mux_addr);//add the multiplexer to the I2C bus
    I2C_add_device(I2C_temp_sens_addr);
    double temp;
    I2C_trigger_measurement();
	while(1){
	I2C_read_temperature(&temp);
	ESP_LOGI("I2C", "[ %lf ]", temp);
	ESP_LOGI("I2C", "\n");
	sendCommandApp(TEMPRATURE, (double*)&temp, DOUBLE);
	vTaskDelay(pdMS_TO_TICKS(2000));
	}

    // add the other devices - uncomment when added
    //I2C_add_device(I2C_temp_sens_addr);
    //I2C_add_device(I2C_distance_sens_addr);
    //I2C_add_device(I2C_pixy2_camera_addr);
    //I2C_add_device(I2C_oled_display_096_addr);
}

void config_rst_pin_i2c_mux() {
	gpio_reset_pin(RST_PIN_MUX_I2C);
	gpio_set_direction(RST_PIN_MUX_I2C, GPIO_MODE_OUTPUT);
}
void rst_pin_i2c_mux_on()
{
	gpio_set_level(RST_PIN_MUX_I2C, 1);
}

void rst_pin_i2c_mux_off()
{
	gpio_set_level(RST_PIN_MUX_I2C, 0);
}

void start_I2C_devices_task()
{
	xTaskCreatePinnedToCore(I2C_devices_task, "I2C_devices_task", 4096, NULL, 7, NULL, 1);
}

/******** BMP/BME280 sensor **********/
//I2C functions for accessing the BMP/BME280 sensor : temperature, pressure, humidity
struct bme280_calib_data temp_calibration;
// I2C temperature sensor BMP functions
void I2C_trigger_measurement(){
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
