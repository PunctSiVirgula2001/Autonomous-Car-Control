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
	esp32_i2c_config.trans_queue_depth = 10; 								// queue size when the communication is asynchronous
	esp32_i2c_config.glitch_ignore_cnt = GLITCH_IGNORE_CNT; 				// number of clock cycles to ignore for glitch filtering
	esp32_i2c_config.intr_priority = 1;
    ESP_ERROR_CHECK(i2c_new_master_bus(&esp32_i2c_config, &bus_handle_esp32_i2c_config));
}

// switch the multiplexer channel
// Function to switch the I2C multiplexer channel
void I2C_select_multiplexer_channel(I2C_devices_mux num_channel)
{
	// Prepare the data to send: one byte where each bit represents a channel
	uint8_t data = 1 << num_channel; // Shift 1 to the correct bit position for the channel
	// Perform the I2C write to change the channel
	esp_err_t ret = i2c_master_transmit(device_handle_esp32_i2c_config[I2C_multiplexer_dev_handle], &data, 1, pdMS_TO_TICKS(100));
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
        .scl_speed_hz = SCL_SPEED_LOW_MODE
    };

    switch (device_address) {
      case I2C_temp_sens_addr:
    	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_temp_sens_mux);
    	  vTaskDelay(pdMS_TO_TICKS(50)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_temp_sens_dev_handle]));
    	  }
    	  break;
      case I2C_distance_sens_addr:
    	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_distance_sens_1_mux);
    	  vTaskDelay(pdMS_TO_TICKS(50)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_distance_sens1_dev_handle])); // distance sensors have the same addr
    	  vTaskDelay(pdMS_TO_TICKS(50)); // space for the switch of the channel to happen
    	  I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);
    	  vTaskDelay(pdMS_TO_TICKS(50)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_distance_sens2_dev_handle])); // distance sensors have the same addr
    	  }
    	  break;
      case I2C_pixy2_camera_addr:
      	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
    	  vTaskDelay(pdMS_TO_TICKS(50)); // space for the switch of the channel to happen
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_pixy2_dev_handle]));
    	  }
    	  break;
      case I2C_oled_display_096_addr:
    	  if(mux_added){
          I2C_select_multiplexer_channel(I2C_oled_display_096_mux);
          vTaskDelay(pdMS_TO_TICKS(50)); // space for the switch of the channel to happen
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


static void i2c_manual_stop(int stop_ms) {
    // Ensure both lines are high before generating a stop condition
    gpio_set_level(I2C_SCL, 1);
    gpio_set_level(I2C_SDA, 0); // SDA low while SCL is high

    // Small delay to meet the timing requirements
    vTaskDelay(pdMS_TO_TICKS(stop_ms)); // Use an appropriate delay based on your I2C clock speed

    // Generate stop condition
    gpio_set_level(I2C_SCL, 1);
    gpio_set_level(I2C_SDA, 1); // Transition SDA to high while SCL is high
}
void I2C_transmit(I2C_dev_handles device_handle, unsigned char* data, size_t write_size)
{
	static I2C_dev_handles old_handle;
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
	ESP_ERROR_CHECK(i2c_master_transmit(device_handle_esp32_i2c_config[device_handle], (unsigned char*)data, write_size, pdMS_TO_TICKS(1000))); // 3rd argument = length of the data in bytes ==> only commands, 2 bytes only
    i2c_manual_stop(5);
}

void I2C_receive(I2C_dev_handles device_handle, uint8_t* data, size_t read_size)
{

	static I2C_dev_handles old_handle=99;
	static I2C_dev_handles current_handle;

	current_handle=device_handle;
	// If the channel is already selected, don't call the function for changing
	// channel.
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

	vTaskDelay(pdMS_TO_TICKS(20)); // space for the switch of the channel to happen
	esp_err_t ret = i2c_master_receive(device_handle_esp32_i2c_config[device_handle], (unsigned char*)data, read_size, pdMS_TO_TICKS(1000));
	if (ret != ESP_OK) {
	    ESP_LOGE("I2C Receive", "Failed to receive data: %s", esp_err_to_name(ret));
	}
    i2c_manual_stop(5);

}

uint8_t I2C_readRegister8bit(I2C_dev_handles device_handle, uint8_t reg)
{
    uint8_t data;
    I2C_transmit(device_handle, &reg, 1);
    vTaskDelay(pdMS_TO_TICKS(30));
    ESP_LOGI(" ","aAFGDSG");
    i2c_manual_stop(10);
    I2C_receive(device_handle, &data, 1);
    vTaskDelay(pdMS_TO_TICKS(30));
    return data;
}

uint16_t I2C_readRegister16bit(I2C_dev_handles device_handle, uint8_t reg)
{
    uint8_t data[2];
    uint16_t data_f;
    I2C_transmit(device_handle, &reg, 1);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Read two bytes at once
    I2C_receive(device_handle, data, 2);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Combine the two bytes into one 16-bit value
    data_f = (uint16_t)data[0] << 8 | data[1];

    return data_f;
}

uint32_t I2C_readRegister32bit(I2C_dev_handles device_handle, uint8_t reg)
{
    uint8_t data[4];   // Array to hold the four bytes read from the device
    uint32_t data_f = 0; // Initialize the final 32-bit data value

    // Send the register address from which to start the read
    I2C_transmit(device_handle, &reg, 1);
    vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow for address setup

    // Receive 4 bytes of data
    I2C_receive(device_handle, data, 4);
    vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow the read transaction to complete

    // Convert the 4 bytes into a single 32-bit integer (assume big-endian order)
    data_f = (uint32_t)data[0] << 24; // Highest byte
    data_f |= (uint32_t)data[1] << 16; // Higher middle byte
    data_f |= (uint32_t)data[2] << 8;  // Lower middle byte
    data_f |= data[3];                 // Lowest byte

    return data_f;
}


void I2C_writeRegister8bit(I2C_dev_handles device_handle, uint8_t reg, uint8_t value)
{
	uint8_t data[2];
	data[0] = reg;   // First byte is the register address
	data[1] = value; // Second byte is the data to write

	// Transmit both bytes at once
	I2C_transmit(device_handle, data, 1);
	//i2c_manual_stop(pdMS_TO_TICKS(5));
	//vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow the transaction to complete
}

void I2C_writeRegister16bit(I2C_dev_handles device_handle, uint8_t reg, uint16_t value)
{
    uint8_t data[3]; // Array to hold the register and the two bytes of the value
    data[0] = reg;               // Register address
    data[1] = (uint8_t)(value >> 8); // High byte of the 16-bit value
    data[2] = (uint8_t)value;        // Low byte of the 16-bit value

    // Transmit the register address and value in one go
    I2C_transmit(device_handle, data, 2);
    vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow the transaction to complete
}

void I2C_writeRegister32bit(I2C_dev_handles device_handle, uint8_t reg, uint32_t value)
{
    uint8_t data[5]; // Array to hold the register and the four bytes of the value
    data[0] = reg;                  // Register address
    data[1] = (uint8_t)(value >> 24); // Highest byte
    data[2] = (uint8_t)(value >> 16); // Higher middle byte
    data[3] = (uint8_t)(value >> 8);  // Lower middle byte
    data[4] = (uint8_t)value;         // Lowest byte

    // Transmit the register address and value in one go
    I2C_transmit(device_handle, data, 4);
    vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow the transaction to complete
}


void I2C_probe(I2C_devices device_addr)
{
	//esp_err_t a =i2c_master_probe(bus_handle_esp32_i2c_config,device_addr,1000); // 1000ms timeout

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
    I2C_add_device(I2C_distance_sens_addr);

    double temp;
    //vTaskDelay(pdMS_TO_TICKS(500));
    //I2C_trigger_measurement();
    //I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);

    VL53L0X_Init(I2C_distance_sens1_dev_handle);
	while(1){
	uint16_t val = VL53L0X_readRangeSingleMillimeters(I2C_distance_sens1_dev_handle);

	ESP_LOGI("Range: ","%"PRIu16, val);
//	I2C_read_temperature(&temp);
//	ESP_LOGI("I2C", "[ %lf ]", temp);
//	ESP_LOGI("I2C", "\n");
//	//sendCommandApp(TEMPRATURE, (double*)&temp, DOUBLE);
	vTaskDelay(pdMS_TO_TICKS(500));
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

/******** VLX sensor **********/
uint8_t stop_variable;
uint32_t measurement_timing_budget_us;
bool VL53L0X_getSpadInfo(I2C_dev_handles device_handle ,uint8_t * count, bool * type_is_aperture)
{
  uint8_t tmp;

  I2C_writeRegister8bit(device_handle, 0x80, 0x01);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
  I2C_writeRegister8bit(device_handle, 0x00, 0x00);

  I2C_writeRegister8bit(device_handle, 0xFF, 0x06);
  I2C_writeRegister8bit(device_handle, 0x83, I2C_readRegister8bit(device_handle, 0x83) | 0x04);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x07);
  I2C_writeRegister8bit(device_handle, 0x81, 0x01);

  I2C_writeRegister8bit(device_handle, 0x80, 0x01);

  I2C_writeRegister8bit(device_handle, 0x94, 0x6b);
  I2C_writeRegister8bit(device_handle, 0x83, 0x00);
  //startTimeout();
  while (I2C_readRegister8bit(device_handle, 0x83) == 0x00)
  {
    //if (checkTimeoutExpired()) { return false; }
	ESP_LOGI(" ","TIMEOUT");
	vTaskDelay(pdMS_TO_TICKS(100));
  }
  I2C_writeRegister8bit(device_handle, 0x83, 0x01);
  tmp = I2C_readRegister8bit(device_handle, 0x92);

  *count = tmp & 0x7f;
  *type_is_aperture = (tmp >> 7) & 0x01;

  I2C_writeRegister8bit(device_handle, 0x81, 0x00);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x06);
  uint8_t data;
  I2C_transmit(device_handle, (unsigned char[]){0x83},1);
  vTaskDelay(pdMS_TO_TICKS(10));
  I2C_receive(device_handle, &data,1);
  vTaskDelay(pdMS_TO_TICKS(10));
  I2C_writeRegister8bit(device_handle, 0x83, data  & ~0x04);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
  I2C_writeRegister8bit(device_handle, 0x00, 0x01);

  I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
  I2C_writeRegister8bit(device_handle, 0x80, 0x00);

  return true;
}



bool VL53L0X_Init(I2C_dev_handles device_handle)
{

	//if(I2C_readRegister8bit(device_handle, IDENTIFICATION_MODEL_ID)!=0xEE)
	//	return false;
	  //i2c_manual_stop(5);VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV) | 0x01)
	  uint8_t ID = I2C_readRegister8bit(device_handle, IDENTIFICATION_MODEL_ID);
	  ESP_LOGI(" ", "ID: %u\n", ID);
	  //i2c_manual_stop(pdMS_TO_TICKS(5));
	 // "Set I2C standard mode"
	  I2C_writeRegister8bit(device_handle,0x88, 0x00);
	  I2C_writeRegister8bit(device_handle,0x80, 0x01);
	  I2C_writeRegister8bit(device_handle,0xFF, 0x01);
	  I2C_writeRegister8bit(device_handle,0x00, 0x00);
	  uint8_t data;
	  I2C_transmit(device_handle, (unsigned char[]){0x91},1);
	  //vTaskDelay(pdMS_TO_TICKS(100));
	  I2C_receive(device_handle, &data,1);
	  //vTaskDelay(pdMS_TO_TICKS(100));
	  ESP_LOGI(" ", "Data: %u\n", data);
	  //s_i2c_hw_fsm_reset(bus_handle_esp32_i2c_config);
	  //stop_variable = I2C_readRegister8bit(device_handle, 0x91);
	  stop_variable = data;
	  i2c_manual_stop(pdMS_TO_TICKS(1000));
	  ESP_LOGI("Set I2C standard mode", "\n");
	  I2C_writeRegister8bit(device_handle,0x00, 0x01);
	  ESP_LOGI("Set I2C standard mode", "\n");
	  I2C_writeRegister8bit(device_handle,0xFF, 0x00);
	  ESP_LOGI("Set I2C standard mode", "\n");
	  I2C_writeRegister8bit(device_handle,0x80, 0x00);

	  //vTaskDelay(pdMS_TO_TICKS(2000));
	  // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
	  uint8_t MSRC_CONFIG = (I2C_readRegister8bit(device_handle, MSRC_CONFIG_CONTROL));
	  //vTaskDelay(pdMS_TO_TICKS(2000));
	  I2C_writeRegister8bit(device_handle, MSRC_CONFIG_CONTROL, MSRC_CONFIG | 0x12) ;
	  ESP_LOGI("Disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4)", "\n");

	  uint8_t spad_count;
	  bool spad_type_is_aperture;
	  if (!VL53L0X_getSpadInfo(device_handle ,&spad_count, &spad_type_is_aperture)) { return false; }
	  ESP_LOGI("getSpadInfo", "\n");
	  vTaskDelay(pdMS_TO_TICKS(2000));
	  uint8_t ref_spad_map[6];
	  uint8_t spad_enables_ref_0 = GLOBAL_CONFIG_SPAD_ENABLES_REF_0;
	  I2C_transmit(device_handle, &spad_enables_ref_0, sizeof(spad_enables_ref_0));
	  vTaskDelay(pdMS_TO_TICKS(10));
	  I2C_receive(device_handle, ref_spad_map, 6);
	  vTaskDelay(pdMS_TO_TICKS(10));
//
	  I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	  I2C_writeRegister8bit(device_handle, DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
	  I2C_writeRegister8bit(device_handle, DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
	  I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	  I2C_writeRegister8bit(device_handle, GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

	  uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0; // 12 is the first aperture spad
	  uint8_t spads_enabled = 0;

	   for (uint8_t i = 0; i < 48; i++)
	   {
	     if (i < first_spad_to_enable || spads_enabled == spad_count)
	     {
	       // This bit is lower than the first one that should be enabled, or
	       // (reference_spad_count) bits have already been enabled, so zero this bit
	       ref_spad_map[i / 8] &= ~(1 << (i % 8));
	     }
	     else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1)
	     {
	       spads_enabled++;
	     }
	   }
	   I2C_transmit(device_handle, &spad_enables_ref_0, sizeof(spad_enables_ref_0));
	   vTaskDelay(pdMS_TO_TICKS(10));
	   I2C_transmit(device_handle, ref_spad_map, sizeof(ref_spad_map)); // 6
	   vTaskDelay(pdMS_TO_TICKS(10));

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x00, 0x00);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x09, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x10, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x11, 0x00);

	   I2C_writeRegister8bit(device_handle, 0x24, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x25, 0xFF);
	   I2C_writeRegister8bit(device_handle, 0x75, 0x00);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x4E, 0x2C);
	   I2C_writeRegister8bit(device_handle, 0x48, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x30, 0x20);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x30, 0x09);
	   I2C_writeRegister8bit(device_handle, 0x54, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x31, 0x04);
	   I2C_writeRegister8bit(device_handle, 0x32, 0x03);
	   I2C_writeRegister8bit(device_handle, 0x40, 0x83);
	   I2C_writeRegister8bit(device_handle, 0x46, 0x25);
	   I2C_writeRegister8bit(device_handle, 0x60, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x27, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x50, 0x06);
	   I2C_writeRegister8bit(device_handle, 0x51, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x52, 0x96);
	   I2C_writeRegister8bit(device_handle, 0x56, 0x08);
	   I2C_writeRegister8bit(device_handle, 0x57, 0x30);
	   I2C_writeRegister8bit(device_handle, 0x61, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x62, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x64, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x65, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x66, 0xA0);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x22, 0x32);
	   I2C_writeRegister8bit(device_handle, 0x47, 0x14);
	   I2C_writeRegister8bit(device_handle, 0x49, 0xFF);
	   I2C_writeRegister8bit(device_handle, 0x4A, 0x00);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x7A, 0x0A);
	   I2C_writeRegister8bit(device_handle, 0x7B, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x78, 0x21);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x23, 0x34);
	   I2C_writeRegister8bit(device_handle, 0x42, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x44, 0xFF);
	   I2C_writeRegister8bit(device_handle, 0x45, 0x26);
	   I2C_writeRegister8bit(device_handle, 0x46, 0x05);
	   I2C_writeRegister8bit(device_handle, 0x40, 0x40);
	   I2C_writeRegister8bit(device_handle, 0x0E, 0x06);
	   I2C_writeRegister8bit(device_handle, 0x20, 0x1A);
	   I2C_writeRegister8bit(device_handle, 0x43, 0x40);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x34, 0x03);
	   I2C_writeRegister8bit(device_handle, 0x35, 0x44);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x31, 0x04);
	   I2C_writeRegister8bit(device_handle, 0x4B, 0x09);
	   I2C_writeRegister8bit(device_handle, 0x4C, 0x05);
	   I2C_writeRegister8bit(device_handle, 0x4D, 0x04);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x44, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x45, 0x20);
	   I2C_writeRegister8bit(device_handle, 0x47, 0x08);
	   I2C_writeRegister8bit(device_handle, 0x48, 0x28);
	   I2C_writeRegister8bit(device_handle, 0x67, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x70, 0x04);
	   I2C_writeRegister8bit(device_handle, 0x71, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x72, 0xFE);
	   I2C_writeRegister8bit(device_handle, 0x76, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x77, 0x00);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x0D, 0x01);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x80, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x01, 0xF8);

	   I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x8E, 0x01);
	   I2C_writeRegister8bit(device_handle, 0x00, 0x01);
	   I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	   I2C_writeRegister8bit(device_handle, 0x80, 0x00);

	   I2C_writeRegister8bit(device_handle, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
	   ESP_LOGI(" ","VL53L0X_setMeasurementTimingBudget1 \n");
	   I2C_writeRegister8bit(device_handle, GPIO_HV_MUX_ACTIVE_HIGH, I2C_readRegister8bit(device_handle, GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10); // active low
	   ESP_LOGI(" ","VL53L0X_setMeasurementTimingBudget2 \n");
	   I2C_writeRegister8bit(device_handle, SYSTEM_INTERRUPT_CLEAR, 0x01);
	   ESP_LOGI(" ","VL53L0X_setMeasurementTimingBudget3 \n");
	   measurement_timing_budget_us = VL53L0X_getMeasurementTimingBudget(device_handle);
	   ESP_LOGI(" ","VL53L0X_setMeasurementTimingBudget4 \n");

	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0xE8);
	   ESP_LOGI(" ","VL53L0X_setMeasurementTimingBudget5 \n");
	   VL53L0X_setMeasurementTimingBudget(device_handle,measurement_timing_budget_us);

	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0x01);

	   if (!VL53L0X_performSingleRefCalibration(device_handle, 0x40)) { return false; }

	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0x02);
	   if (!VL53L0X_performSingleRefCalibration(device_handle, 0x00)) { return false; }

	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0xE8);
	   ESP_LOGI(" ","VLX sensor init done! \n");
	   return true;
}

uint32_t VL53L0X_getMeasurementTimingBudget(I2C_dev_handles device_handle)
{
  SequenceStepEnables enables;
  SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead     = 1910;
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  // "Start and end overhead times always present"
  uint32_t budget_us = StartOverhead + EndOverhead;

  VL53L0X_getSequenceStepEnables(device_handle , &enables);
  VL53L0X_getSequenceStepTimeouts(device_handle, &enables, &timeouts);

  if (enables.tcc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    budget_us += (timeouts.final_range_us + FinalRangeOverhead);
  }

  measurement_timing_budget_us = budget_us; // store for internal reuse
  return budget_us;
}

bool VL53L0X_setMeasurementTimingBudget(I2C_dev_handles device_handle, uint32_t budget_us)
{
  SequenceStepEnables enables;
  SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead     = 1910;
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  uint32_t used_budget_us = StartOverhead + EndOverhead;

  VL53L0X_getSequenceStepEnables(device_handle,&enables);
  VL53L0X_getSequenceStepTimeouts(device_handle,&enables, &timeouts);

  if (enables.tcc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    used_budget_us += FinalRangeOverhead;

    // "Note that the final range timeout is determined by the timing
    // budget and the sum of all other timeouts within the sequence.
    // If there is no room for the final range timeout, then an error
    // will be set. Otherwise the remaining time will be applied to
    // the final range."

    if (used_budget_us > budget_us)
    {
      // "Requested timeout too big."
      return false;
    }

    uint32_t final_range_timeout_us = budget_us - used_budget_us;
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

    // "For the final range timeout, the pre-range timeout
    //  must be added. To do this both final and pre-range
    //  timeouts must be expressed in macro periods MClks
    //  because they have different vcsel periods."
    uint32_t final_range_timeout_mclks =0;
    //VL53L0X_timeoutMicrosecondsToMclks(final_range_timeout_us,
                               // timeouts.final_range_vcsel_period_pclks);

    if (enables.pre_range)
    {
      final_range_timeout_mclks += timeouts.pre_range_mclks;
    }

    I2C_writeRegister8bit(device_handle, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
    VL53L0X_encodeTimeout(final_range_timeout_mclks));

    // set_sequence_step_timeout() end

    measurement_timing_budget_us = budget_us; // store for internal reuse
  }
  return true;
}


void VL53L0X_getSequenceStepEnables(I2C_dev_handles device_handle, SequenceStepEnables * enables)
{
  uint8_t sequence_config = I2C_readRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG);

  enables->tcc          = (sequence_config >> 4) & 0x1;
  enables->dss          = (sequence_config >> 3) & 0x1;
  enables->msrc         = (sequence_config >> 2) & 0x1;
  enables->pre_range    = (sequence_config >> 6) & 0x1;
  enables->final_range  = (sequence_config >> 7) & 0x1;
}

void VL53L0X_getSequenceStepTimeouts(I2C_dev_handles device_handle, SequenceStepEnables const * enables, SequenceStepTimeouts * timeouts)
{
  timeouts->pre_range_vcsel_period_pclks = VL53L0X_getVcselPulsePeriod(device_handle, VcselPeriodPreRange);

  timeouts->msrc_dss_tcc_mclks = I2C_readRegister8bit(device_handle, MSRC_CONFIG_TIMEOUT_MACROP) + 1;
  timeouts->msrc_dss_tcc_us =
    VL53L0X_timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

  timeouts->pre_range_mclks =
    VL53L0X_decodeTimeout(I2C_readRegister16bit(device_handle, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
  timeouts->pre_range_us =
    VL53L0X_timeoutMclksToMicroseconds(timeouts->pre_range_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

  timeouts->final_range_vcsel_period_pclks = VL53L0X_getVcselPulsePeriod(device_handle, VcselPeriodFinalRange);

  timeouts->final_range_mclks =
    VL53L0X_decodeTimeout(I2C_readRegister16bit(device_handle, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));

  if (enables->pre_range)
  {
    timeouts->final_range_mclks -= timeouts->pre_range_mclks;
  }

  timeouts->final_range_us =
    VL53L0X_timeoutMclksToMicroseconds(timeouts->final_range_mclks,
                               timeouts->final_range_vcsel_period_pclks);
}

// Get the VCSEL pulse period in PCLKs for the given period type.
// based on VL53L0X_get_vcsel_pulse_period()
uint8_t VL53L0X_getVcselPulsePeriod(I2C_dev_handles device_handle, vcselPeriodType type)
{
  if (type == VcselPeriodPreRange)
  {
    return decodeVcselPeriod(I2C_readRegister8bit(device_handle, PRE_RANGE_CONFIG_VCSEL_PERIOD));
  }
  else if (type == VcselPeriodFinalRange)
  {
    return decodeVcselPeriod(I2C_readRegister8bit(device_handle, FINAL_RANGE_CONFIG_VCSEL_PERIOD));
  }
  else { return 255; }
}

uint32_t VL53L0X_timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);

  return ((timeout_period_mclks * macro_period_ns) + 500) / 1000;
}

uint16_t VL53L0X_decodeTimeout(uint16_t reg_val)
{
  // format: "(LSByte * 2^MSByte) + 1"
  return (uint16_t)((reg_val & 0x00FF) <<
         (uint16_t)((reg_val & 0xFF00) >> 8)) + 1;
}

uint32_t VL53L0X_timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
  return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}

uint16_t VL53L0X_encodeTimeout(uint32_t timeout_mclks)
{
  // format: "(LSByte * 2^MSByte) + 1"

  uint32_t ls_byte = 0;
  uint16_t ms_byte = 0;

  if (timeout_mclks > 0)
  {
    ls_byte = timeout_mclks - 1;

    while ((ls_byte & 0xFFFFFF00) > 0)
    {
      ls_byte >>= 1;
      ms_byte++;
    }

    return (ms_byte << 8) | (ls_byte & 0xFF);
  }
  else { return 0; }
}

bool VL53L0X_performSingleRefCalibration(I2C_dev_handles device_handle ,uint8_t vhv_init_byte)
{
  I2C_writeRegister8bit(device_handle, SYSRANGE_START, 0x01 | vhv_init_byte); // VL53L0X_REG_SYSRANGE_MODE_START_STOP

  //startTimeout();
  while ((I2C_readRegister8bit(device_handle, RESULT_INTERRUPT_STATUS) & 0x07) == 0)
  {
    //if (checkTimeoutExpired()) { return false; }
	  vTaskDelay(pdMS_TO_TICKS(500));
  }

  I2C_writeRegister8bit(device_handle, SYSTEM_INTERRUPT_CLEAR, 0x01);

  I2C_writeRegister8bit(device_handle, SYSRANGE_START, 0x00);

  return true;
}

uint16_t VL53L0X_readRangeSingleMillimeters(I2C_dev_handles device_handle)
{
  I2C_writeRegister8bit(device_handle, 0x80, 0x01);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
  I2C_writeRegister8bit(device_handle, 0x00, 0x00);
  I2C_writeRegister8bit(device_handle, 0x91, stop_variable);
  I2C_writeRegister8bit(device_handle, 0x00, 0x01);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
  I2C_writeRegister8bit(device_handle, 0x80, 0x00);

  I2C_writeRegister8bit(device_handle, SYSRANGE_START, 0x01);
  ESP_LOGI(" ", "AICI");
   //"Wait until start bit has been cleared"
  //startTimeout();
  while ((I2C_readRegister8bit(device_handle, SYSRANGE_START) & 0x01))
  {
	  ESP_LOGI(" ", "AICI %u", I2C_readRegister8bit(device_handle, SYSRANGE_START) & 0x01 );
//    if (checkTimeoutExpired())
//    {
//      did_timeout = true;
//      return 65535;
//    }
  }

  return VL53L0X_readRangeContinuousMillimeters(device_handle);
}

uint16_t VL53L0X_readRangeContinuousMillimeters(I2C_dev_handles device_handle)
{
  //startTimeout();
  while ((I2C_readRegister8bit(device_handle, RESULT_INTERRUPT_STATUS) & 0x07) == 0)
  {
//    if (checkTimeoutExpired())
//    {
//      did_timeout = true;
//      return 65535;
//    }
  }

  // assumptions: Linearity Corrective Gain is 1000 (default);
  // fractional ranging is not enabled
  uint16_t range = I2C_readRegister16bit(device_handle, RESULT_RANGE_STATUS + 10);

  I2C_writeRegister8bit(device_handle, SYSTEM_INTERRUPT_CLEAR, 0x01);

  return range;
}

