#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_task_wdt.h"
#include "driver/i2c_master.h"
// I2C config
#define I2C_SDA 21
#define I2C_SCL 22
#define INTERNAL_PULLUPS false
#define GLITCH_IGNORE_CNT 7
#define SCL_SPEED_FAST_MODE 400000
#define SCL_SPEED_SLOW_MODE 100000
#define RST_PIN_MUX_I2C 23
#define I2C_COMMAND_SIZE (sizeof(I2C_COMMAND))
#define QUEUE_SIZE_I2C 10



// I2C 8 devices connected to the I2C multiplexer. --> control register mux
typedef enum I2C_devices_multiplexer {
	NO_MUX_SELECTED = -1,
    I2C_distance_sens_1_mux  = 0,
    I2C_distance_sens_2_mux  = 1,
	I2C_adxl345_sens_mux 	 = 3,
	I2C_temp_sens_mux		 = 2,
	I2C_oled_display_096_mux = 4,
    I2C_pixy2_camera_mux 	 = 5,
}I2C_devices_mux;

// Each device connected to the car that works with I2C. --> device address
typedef enum I2C_devices{
	I2C_mux_addr			  = 0x70, // 1110 000 0/1 transmit/receive
	I2C_temp_sens_addr		  = 0x76,
	I2C_distance_sens_addr    = 0x29,
	I2C_pixy2_camera_addr 	  = 0x54,
	I2C_adxl345_sens_addr 	  = 0x53,
	I2C_oled_display_096_addr = 0x00
}I2C_devices;

typedef enum I2C_dev_handles
{
  I2C_multiplexer_dev_handle,
  I2C_pixy2_dev_handle,
  I2C_distance_sens1_dev_handle, // fata
  I2C_distance_sens2_dev_handle, // spate
  I2C_temp_sens_dev_handle,
  I2C_oled_display_096_dev_handle,
  I2C_adxl345_sens_dev_handle,
  I2C_MAX_Num_of_dev_handles
}I2C_dev_handles;

typedef enum I2C_WRR_tokens
{
	pixy2 = 0,
	distance_sens1 = 3,
	distance_sens2 = 2,
	adxl_acc = 2,
	temp_sens = 1
}I2C_WRR_tokens;

typedef enum {
    SENSOR_IDLE,
    SENSOR_READING,
    SENSOR_READY
} sensor_state_t;

typedef enum
{
  I2C_NO_STATE,
  I2C_STOP_MOTOR,
  I2C_START_MOTOR,
  I2C_NEW_STEER,
  I2C_MOTOR_OFFSET_ACCELEROMETER
}I2C_COMMAND_TYPE;

typedef struct {
    I2C_dev_handles device_handle;
    I2C_devices_mux mux;
    I2C_WRR_tokens tokens;
    I2C_WRR_tokens tokens_remaining;
    sensor_state_t state;
} sensor_t;

typedef struct I2C_COMMAND{
  I2C_dev_handles sendingSensor;
  I2C_COMMAND_TYPE command;
  uint16_t commandValue;
}I2C_COMMAND;

// Init I2C for esp32 as master
void I2C_master_init();

void I2C_select_multiplexer_channel(I2C_devices_mux num_channel);

void I2C_transmit(I2C_dev_handles device_handle, unsigned char* data, size_t write_size);

void I2C_receive(I2C_dev_handles device_handle, uint8_t* data, size_t read_size);

uint8_t I2C_readRegister8bit(I2C_dev_handles device_handle,uint8_t reg);

uint16_t I2C_readRegister16bit(I2C_dev_handles device_handle, uint8_t reg);

uint32_t I2C_readRegister32bit(I2C_dev_handles device_handle, uint8_t reg);

void I2C_writeRegister8bit(I2C_dev_handles device_handle, uint8_t reg, uint8_t value);

void I2C_writeRegister16bit(I2C_dev_handles device_handle, uint8_t reg, uint16_t value);

void I2C_writeRegister32bit(I2C_dev_handles device_handle, uint8_t reg, uint32_t value);

void I2C_add_device(uint8_t device_address);

void start_I2C_devices_task();

void config_rst_pin_i2c_mux();

void rst_pin_i2c_mux_on();

void rst_pin_i2c_mux_off();

/******** BMP/BME280 sensor **********/
struct bme280_calib_data {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    // Add humidity calibration data here if needed
};
// I2C temperature sensor BMP functions
void I2C_trigger_measurement();
// I2C pressure sensor BME functions
void I2C_set_pressure_register();
// I2C humidity sensor BME/BMP functions
void I2C_set_humidity_register();
void I2C_read_temperature(double *fine_temp);
// Task for I2C devices
void I2C_devices_task(void *pvParameters);

/* VL53L0X */
#define ALPHA_VL53L0X 0.8
#define ALPHA_ADXL 0.8
#define Threshold_dist 250.0

static uint8_t stop_variable;
static uint16_t timeout_start_ms;
static uint32_t measurement_timing_budget_us;
#define io_timeout 100
#define millis() (pdTICKS_TO_MS(xTaskGetTickCount()))
#define checkTimeoutExpired() (io_timeout > 0 && ((uint16_t)(millis() - timeout_start_ms)) > io_timeout)
// Record the current time to check an upcoming timeout against
#define startTimeout() (timeout_start_ms = millis())

// Check if timeout is enabled (set to nonzero value) and has expired

#define calcMacroPeriod(vcsel_period_pclks) ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)
#define decodeVcselPeriod(reg_val)      (((reg_val) + 1) << 1)
uint16_t VL53L0X_readDistance(I2C_dev_handles device_handle);
typedef enum dist_sens_registers
{
	  SYSRANGE_START                              = 0x00,
	  SYSTEM_THRESH_HIGH                          = 0x0C,
	  SYSTEM_THRESH_LOW                           = 0x0E,
	  SYSTEM_SEQUENCE_CONFIG                      = 0x01,
	  SYSTEM_RANGE_CONFIG                         = 0x09,
	  SYSTEM_INTERMEASUREMENT_PERIOD              = 0x04,
	  SYSTEM_INTERRUPT_CONFIG_GPIO                = 0x0A,
	  GPIO_HV_MUX_ACTIVE_HIGH                     = 0x84,
	  SYSTEM_INTERRUPT_CLEAR                      = 0x0B,
	  RESULT_INTERRUPT_STATUS                     = 0x13,
	  RESULT_RANGE_STATUS                         = 0x14,
	  RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN       = 0xBC,
	  RESULT_CORE_RANGING_TOTAL_EVENTS_RTN        = 0xC0,
	  RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF       = 0xD0,
	  RESULT_CORE_RANGING_TOTAL_EVENTS_REF        = 0xD4,
	  RESULT_PEAK_SIGNAL_RATE_REF                 = 0xB6,
	  ALGO_PART_TO_PART_RANGE_OFFSET_MM           = 0x28,
	  I2C_SLAVE_DEVICE_ADDRESS                    = 0x8A,
	  MSRC_CONFIG_CONTROL                         = 0x60,
	  PRE_RANGE_CONFIG_MIN_SNR                    = 0x27,
	  PRE_RANGE_CONFIG_VALID_PHASE_LOW            = 0x56,
	  PRE_RANGE_CONFIG_VALID_PHASE_HIGH           = 0x57,
	  PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          = 0x64,
	  FINAL_RANGE_CONFIG_MIN_SNR                  = 0x67,
	  FINAL_RANGE_CONFIG_VALID_PHASE_LOW          = 0x47,
	  FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         = 0x48,
	  FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44,
	  PRE_RANGE_CONFIG_SIGMA_THRESH_HI            = 0x61,
	  PRE_RANGE_CONFIG_SIGMA_THRESH_LO            = 0x62,
	  PRE_RANGE_CONFIG_VCSEL_PERIOD               = 0x50,
	  PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          = 0x51,
	  PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO          = 0x52,
	  SYSTEM_HISTOGRAM_BIN                        = 0x81,
	  HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT       = 0x33,
	  HISTOGRAM_CONFIG_READOUT_CTRL               = 0x55,
	  FINAL_RANGE_CONFIG_VCSEL_PERIOD             = 0x70,
	  FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        = 0x71,
	  FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO        = 0x72,
	  CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       = 0x20,
	  MSRC_CONFIG_TIMEOUT_MACROP                  = 0x46,
	  SOFT_RESET_GO2_SOFT_RESET_N                 = 0xBF,
	  IDENTIFICATION_MODEL_ID                     = 0xC0,
	  IDENTIFICATION_REVISION_ID                  = 0xC2,
	  OSC_CALIBRATE_VAL                           = 0xF8,
	  GLOBAL_CONFIG_VCSEL_WIDTH                   = 0x32,
	  GLOBAL_CONFIG_SPAD_ENABLES_REF_0            = 0xB0,
	  GLOBAL_CONFIG_SPAD_ENABLES_REF_1            = 0xB1,
	  GLOBAL_CONFIG_SPAD_ENABLES_REF_2            = 0xB2,
	  GLOBAL_CONFIG_SPAD_ENABLES_REF_3            = 0xB3,
	  GLOBAL_CONFIG_SPAD_ENABLES_REF_4            = 0xB4,
	  GLOBAL_CONFIG_SPAD_ENABLES_REF_5            = 0xB5,
	  GLOBAL_CONFIG_REF_EN_START_SELECT           = 0xB6,
	  DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         = 0x4E,
	  DYNAMIC_SPAD_REF_EN_START_OFFSET            = 0x4F,
	  POWER_MANAGEMENT_GO1_POWER_FORCE            = 0x80,
	  VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV           = 0x89,
	  ALGO_PHASECAL_LIM                           = 0x30,
	  ALGO_PHASECAL_CONFIG_TIMEOUT                = 0x30,
}dist_sens_registers;

typedef struct SequenceStepEnables
{
	bool tcc;
	bool dss;
	bool msrc;
	bool pre_range;
	bool final_range;
} SequenceStepEnables;

typedef struct SequenceStepTimeouts
{
  uint16_t pre_range_vcsel_period_pclks, final_range_vcsel_period_pclks;

  uint16_t msrc_dss_tcc_mclks, pre_range_mclks, final_range_mclks;
  uint32_t msrc_dss_tcc_us,    pre_range_us,    final_range_us;
} SequenceStepTimeouts;

typedef enum vcselPeriodType
{
	VcselPeriodPreRange,
	VcselPeriodFinalRange
} vcselPeriodType;

bool VL53L0X_Init(I2C_dev_handles device_handle);
bool VL53L0X_getSpadInfo(I2C_dev_handles device_handle, uint8_t * count, bool * type_is_aperture);
uint32_t VL53L0X_timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks);
uint8_t VL53L0X_getVcselPulsePeriod(I2C_dev_handles device_handle, vcselPeriodType type);
uint16_t VL53L0X_decodeTimeout(uint16_t reg_val);
void VL53L0X_getSequenceStepTimeouts(I2C_dev_handles device_handle, SequenceStepEnables const * enables, SequenceStepTimeouts * timeouts);
uint32_t VL53L0X_getMeasurementTimingBudget(I2C_dev_handles device_handle);
void VL53L0X_getSequenceStepEnables(I2C_dev_handles device_handle, SequenceStepEnables * enables);
uint32_t VL53L0X_timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks);
uint16_t VL53L0X_encodeTimeout(uint32_t timeout_mclks);
bool VL53L0X_setMeasurementTimingBudget(I2C_dev_handles device_handle, uint32_t budget_us);
bool VL53L0X_performSingleRefCalibration(I2C_dev_handles device_handle ,uint8_t vhv_init_byte);
uint16_t VL53L0X_readRangeContinuousMillimeters(I2C_dev_handles device_handle);
uint16_t VL53L0X_readRangeSingleMillimeters(I2C_dev_handles device_handle);
void VL53L0X_SetInterruptThresholds(I2C_dev_handles device_handle, uint32_t ThresholdLow ,uint32_t ThresholdHigh);

/* ADXL 345 accelerometer sensor */
#define ADXL1345_READ_RATE 200.0 // HZ
#define ALPHA 0.8

typedef enum ADX_345_reg
{
  ADXL345_REG_BW_RATE 	  = 0x2C,
  ADXL345_REG_POWER_CTL   = 0x2D,
  ADXL345_REG_DATA_FORMAT = 0x31,
  ADXL345_REG_DATAX0      = 0x32,
  ADXL345_REG_DATAX1	  = 0x33,
  ADXL345_REG_DATAY0	  = 0x34,
  ADXL345_REG_DATAY1	  = 0x35,
  ADXL345_REG_DATAZ0	  = 0x36,
  ADXL345_REG_DATAZ1	  = 0x37
}ADX_345_reg;

void I2C_read_adxl345_data(I2C_dev_handles device_handle, double* x, double* y, double* z);
void I2C_adxl345_init(I2C_dev_handles device_handle);
void I2C_adxl345_setRate(I2C_dev_handles device_handle, double rate);

