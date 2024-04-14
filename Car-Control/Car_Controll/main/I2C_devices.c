#include "I2C_devices.h"

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
	esp32_i2c_config.flags.enable_internal_pullup = ALLOW_INTERNAL_PULLUPS; // might need external pullups as well
	esp32_i2c_config.trans_queue_depth = 4; 								// queue size when the communication is asynchronous
	esp32_i2c_config.glitch_ignore_cnt = GLITCH_IGNORE_CNT; 				// number of clock cycles to ignore for glitch filtering
	esp32_i2c_config.intr_priority = 0;
    ESP_ERROR_CHECK(i2c_new_master_bus(&esp32_i2c_config, &bus_handle_esp32_i2c_config));
}

// switch the multiplexer channel
void I2C_select_multiplexer_channel(uint8_t num_channel)
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
	   // Handle error here
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
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_temp_sens_dev_handle]));
    	  }
    	  break;
      case I2C_distance_sens_addr:
    	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_distance_sens_1_mux);
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_distance_sens1_dev_handle])); // distance sensors have the same addr
    	  I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_distance_sens2_dev_handle])); // distance sensors have the same addr
    	  }
    	  break;
      case I2C_pixy2_camera_addr:
      	  if(mux_added){
    	  I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
    	  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle_esp32_i2c_config, &device_config, &device_handle_esp32_i2c_config[I2C_pixy2_dev_handle]));
    	  }
    	  break;
      case I2C_oled_display_096_addr:
    	  if(mux_added){
          I2C_select_multiplexer_channel(I2C_oled_display_096_mux);
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


void I2C_transmit(I2C_dev_handles device_handle, unsigned char* data)
{
	//Before transmitting choose the correct channel for the mux
	switch (device_handle) {
		case I2C_multiplexer_dev_handle:
			// do nothing
		break;
		case I2C_pixy2_dev_handle:
			 I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
		break;
		case I2C_distance_sens1_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_1_mux);
		break;
		case I2C_distance_sens2_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);
		break;
		case I2C_temp_sens_dev_handle:
			I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
		break;
		case I2C_oled_display_096_dev_handle:
			I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
		break;
		default:
	}
	ESP_ERROR_CHECK(i2c_master_transmit(device_handle_esp32_i2c_config[device_handle], (unsigned char*)data, 2, -1)); // 3rd argument = length of the data in bytes ==> only commands, 2 bytes only
}

void I2C_receive(I2C_dev_handles device_handle, uint8_t* data)
{
	switch (device_handle) {
		case I2C_multiplexer_dev_handle:
			// do nothing
		break;
		case I2C_pixy2_dev_handle:
			 I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
		break;
		case I2C_distance_sens1_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_1_mux);
		break;
		case I2C_distance_sens2_dev_handle:
			I2C_select_multiplexer_channel(I2C_distance_sens_2_mux);
		break;
		case I2C_temp_sens_dev_handle:
			I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
		break;
		case I2C_oled_display_096_dev_handle:
			I2C_select_multiplexer_channel(I2C_pixy2_camera_mux);
		break;
		default:
	}
	ESP_ERROR_CHECK(i2c_master_receive(device_handle_esp32_i2c_config[device_handle], (uint8_t*)data, 10, 1000)); // 3rd argument = length of the data in bytes ==> only commands, 2 bytes only
}

void I2C_probe(I2C_devices device_addr)
{
	i2c_master_probe(bus_handle_esp32_i2c_config,device_addr,1000); // 1000ms timeout
}

// Task for I2C devices
void I2C_devices_task(void *pvParameters)
{
	//config rst pin for the mux
	config_rst_pin_i2c_mux();
	//rst pin on
	rst_pin_i2c_mux_on();
	//init master
    I2C_master_init();
    // add the multiplexer to the I2C bus
    I2C_add_device(I2C_mux_addr);
	//uint8_t data[10];
	uint8_t channel = 0;
	while(1){
	I2C_select_multiplexer_channel(channel);
	channel++;
	if(channel>4)
	channel = 0;
    // delay
	vTaskDelay(10);
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
