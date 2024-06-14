#include "Network.h"
#include "math.h"
#include "I2C_common.h"


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
	esp32_i2c_config.trans_queue_depth = 0; 								// queue size when the communication is asynchronous
	esp32_i2c_config.glitch_ignore_cnt = GLITCH_IGNORE_CNT; 				// number of clock cycles to ignore for glitch filtering
	esp32_i2c_config.intr_priority = 0;
    ESP_ERROR_CHECK(i2c_new_master_bus(&esp32_i2c_config, &bus_handle_esp32_i2c_config));
}

// switch the multiplexer channel
// Function to switch the I2C multiplexer channel
void I2C_select_multiplexer_channel(I2C_devices_mux num_channel)
{
	// Prepare the data to send: one byte where each bit represents a channel
	uint8_t data = 1 << num_channel; // Shift 1 to the correct bit position for the channel
	// Perform the I2C write to change the channel
	esp_err_t ret = i2c_master_transmit(device_handle_esp32_i2c_config[I2C_multiplexer_dev_handle], &data, 1, -1);
	// Check the result
	//if(ret==ESP_OK)
		//ESP_LOGI("I2C", "Channel %d selected", num_channel);
	// Check the result
	if (ret != ESP_OK)
	{
		ESP_LOGI("I2C ERR", "ERROR Channel %d not selected", num_channel);
		i2c_master_bus_wait_all_done(bus_handle_esp32_i2c_config, 50);
		//i2c_master_bus_reset(bus_handle_esp32_i2c_config);
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
      case I2C_adxl345_sens_addr:
    	  I2C_select_multiplexer_channel(I2C_adxl345_sens_mux);
		  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_adxl345_sens_dev_handle]));
		break;
      default:
    }
}

void I2C_transmit(I2C_dev_handles device_handle, unsigned char* data, size_t write_size)
{
	static I2C_dev_handles old_handle=999;
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
		case I2C_adxl345_sens_dev_handle:
			I2C_select_multiplexer_channel(I2C_adxl345_sens_mux);
			old_handle=I2C_adxl345_sens_dev_handle;
		break;
		default:
	 }
	}
	vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
	esp_err_t ret = i2c_master_transmit(device_handle_esp32_i2c_config[device_handle], (unsigned char*)data, write_size, -1); // 3rd argument = length of the data in bytes ==> only commands, 2 bytes only
	if (ret != ESP_OK) {
		    ESP_LOGE("I2C Receive", "Failed to receive data: %s", esp_err_to_name(ret));
		    i2c_master_bus_wait_all_done(bus_handle_esp32_i2c_config, 50);
		   // i2c_master_bus_reset(bus_handle_esp32_i2c_config);
		}
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
		case I2C_adxl345_sens_dev_handle:
			I2C_select_multiplexer_channel(I2C_adxl345_sens_mux);
			old_handle=I2C_adxl345_sens_dev_handle;
		break;
		default:
	}

	vTaskDelay(pdMS_TO_TICKS(10)); // space for the switch of the channel to happen
	esp_err_t ret = i2c_master_receive(device_handle_esp32_i2c_config[device_handle], (unsigned char*)data, read_size, -1);
	if (ret != ESP_OK) {
	    ESP_LOGE("I2C Receive", "Failed to receive data: %s", esp_err_to_name(ret));
	    i2c_master_bus_wait_all_done(bus_handle_esp32_i2c_config, 100);
	    //i2c_master_bus_reset(bus_handle_esp32_i2c_config);
	}
}


void writeMultipleRegisters(I2C_dev_handles device_handle, const uint8_t (*regs_vals)[2], size_t count) {
    for (size_t i = 0; i < count; i++) {
        I2C_writeRegister8bit(device_handle, regs_vals[i][0], regs_vals[i][1]);
    }
}

uint8_t I2C_readRegister8bit(I2C_dev_handles device_handle, uint8_t reg)
{
    uint8_t data;
    I2C_transmit(device_handle, &reg, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    I2C_receive(device_handle, &data, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    return data;
}

uint16_t I2C_readRegister16bit(I2C_dev_handles device_handle, uint8_t reg)
{
    uint8_t data[2];
    uint16_t data_f;
    I2C_transmit(device_handle, &reg, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    // Read two bytes at once
    I2C_receive(device_handle, data, 2);
    vTaskDelay(pdMS_TO_TICKS(10));

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
	I2C_transmit(device_handle, data, 2);
	//i2c_manual_stop(pdMS_TO_TICKS(5));
	vTaskDelay(pdMS_TO_TICKS(5)); // Delay to allow the transaction to complete
}

void I2C_writeRegister16bit(I2C_dev_handles device_handle, uint8_t reg, uint16_t value)
{
    uint8_t data[3]; // Array to hold the register and the two bytes of the value
    data[0] = reg;               // Register address
    data[1] = (uint8_t)(value >> 8); // High byte of the 16-bit value
    data[2] = (uint8_t)value;        // Low byte of the 16-bit value

    // Transmit the register address and value in one go
    I2C_transmit(device_handle, data, 3);
    //vTaskDelay(pdMS_TO_TICKS(10)); // Delay to allow the transaction to complete
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
    I2C_transmit(device_handle, data, 5);
    vTaskDelay(pdMS_TO_TICKS(50)); // Delay to allow the transaction to complete
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




