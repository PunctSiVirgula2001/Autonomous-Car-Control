/*
 * I2C_distanceSensor.c
 *
 *  Created on: 14 iun. 2024
 *      Author: racov
 */

#include "I2C_distanceSensor.h"

/******** VLX sensor **********/

bool VL53L0X_getSpadInfo(I2C_dev_handles device_handle ,uint8_t * count, bool * type_is_aperture)
{
  uint8_t tmp;

  I2C_writeRegister8bit(device_handle, 0x80, 0x01);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
  I2C_writeRegister8bit(device_handle, 0x00, 0x00);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x06);
  I2C_writeRegister8bit(device_handle, 0x83, (uint8_t)(I2C_readRegister16bit(device_handle, 0x83)>>8) | 0x04);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x07);
  I2C_writeRegister8bit(device_handle, 0x81, 0x01);
  I2C_writeRegister8bit(device_handle, 0x80, 0x01);
  I2C_writeRegister8bit(device_handle, 0x94, 0x6b);
  I2C_writeRegister8bit(device_handle, 0x83, 0x00);
  uint8_t reg;
  startTimeout();
  do
  {
	reg = (uint8_t)(I2C_readRegister16bit(device_handle, 0x83)>>8);
    if (checkTimeoutExpired()) { return false; }
	vTaskDelay(pdMS_TO_TICKS(10));
  } while(reg == 0x00);
  I2C_writeRegister8bit(device_handle, 0x83, 0x01);
  tmp = (uint8_t)(I2C_readRegister16bit(device_handle, 0x92)>>8);

  *count = tmp & 0x7f;
  *type_is_aperture = (tmp >> 7) & 0x01;

  I2C_writeRegister8bit(device_handle, 0x81, 0x00);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x06);
//  uint8_t data;
  I2C_writeRegister8bit(device_handle, 0x83, (uint8_t)(I2C_readRegister16bit(device_handle, 0x83)>>8) & ~0x04);
  I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
  I2C_writeRegister8bit(device_handle, 0x00, 0x01);

  I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
  I2C_writeRegister8bit(device_handle, 0x80, 0x00);

  return true;
}
typedef enum err
{
	not_err=0,
	err=1
}type_error;

static type_error VL53L0X_setSignalRateLimit (I2C_dev_handles device_handle, float limit_Mcps)
{
   if (limit_Mcps < 0 || limit_Mcps > 511.99)
      return err;
   // Q9.7 fixed point format (9 integer bits, 7 fractional bits)
   I2C_writeRegister16bit(device_handle, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, limit_Mcps * (1 << 7));
   return not_err;
}

bool VL53L0X_Init(I2C_dev_handles device_handle)
{
	  if((uint8_t)(I2C_readRegister16bit(device_handle, IDENTIFICATION_MODEL_ID)>>8)!=0xEE)
		return false;

	  uint16_t data;

	 // "Set I2C standard mode"
	  I2C_writeRegister8bit(device_handle,0x88, 0x00);
	  I2C_writeRegister8bit(device_handle,0x80, 0x01);
	  I2C_writeRegister8bit(device_handle,0xFF, 0x01);
	  I2C_writeRegister8bit(device_handle,0x00, 0x00);

	  stop_variable = (uint8_t)(I2C_readRegister16bit(device_handle, 0x91)>>8);
	  ESP_LOGI(" ", "stop_variable: %u", stop_variable);
	  I2C_writeRegister8bit(device_handle,0x00, 0x01);
	  I2C_writeRegister8bit(device_handle,0xFF, 0x00);
	  I2C_writeRegister8bit(device_handle,0x80, 0x00);

	  // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
	  uint8_t MSRC_CONFIG = (uint8_t)(I2C_readRegister16bit(device_handle, MSRC_CONFIG_CONTROL)>>8);
	  I2C_writeRegister8bit(device_handle, MSRC_CONFIG_CONTROL, MSRC_CONFIG | 0x12) ;



	  data = VL53L0X_setSignalRateLimit (device_handle, 0.25);


	  I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0xFF);

	  uint8_t spad_count;
	  bool spad_type_is_aperture;
	  if (!VL53L0X_getSpadInfo(device_handle ,&spad_count, &spad_type_is_aperture)) {
		  //ESP_LOGI(" ","VL53L0X_getSpadInfo false");
		  return false;
	  }
	  uint8_t ref_spad_map[6];
	  uint8_t spad_enables_ref_0 = GLOBAL_CONFIG_SPAD_ENABLES_REF_0;

	  I2C_transmit(device_handle, &spad_enables_ref_0, 1);
	  vTaskDelay(pdMS_TO_TICKS(10));
	  I2C_receive(device_handle, ref_spad_map, 6);
	  vTaskDelay(pdMS_TO_TICKS(10));

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
	   I2C_transmit(device_handle, &spad_enables_ref_0, 1);
	   vTaskDelay(pdMS_TO_TICKS(10));
	   I2C_transmit(device_handle, ref_spad_map, sizeof(ref_spad_map)); // 6

	   VL53L0X_setupI2CRegisters(device_handle);

	   I2C_writeRegister8bit(device_handle, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
	   I2C_writeRegister8bit(device_handle, GPIO_HV_MUX_ACTIVE_HIGH, (uint8_t)(I2C_readRegister16bit(device_handle, GPIO_HV_MUX_ACTIVE_HIGH)>>8) & ~0x10); // active low
	   I2C_writeRegister8bit(device_handle, SYSTEM_INTERRUPT_CLEAR, 0x01);

	   measurement_timing_budget_us = VL53L0X_getMeasurementTimingBudget(device_handle);
	   ESP_LOGI(" ","VL53L0X_setMeasurementTimingBudget %"PRIu32" \n", measurement_timing_budget_us);
	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0xE8);
	   if (!VL53L0X_setMeasurementTimingBudget(device_handle,measurement_timing_budget_us-10)) {
		   //ESP_LOGI(" ","VL53L0X_setMeasurementTimingBudget false \n");
		   return false;
	   };
	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0x01);
	   if (!VL53L0X_performSingleRefCalibration(device_handle, 0x40)) {
		   //ESP_LOGI(" ","VL53L0X_performSingleRefCalibration1 false \n");
		   return false;
	   }
	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0x02);
	   if (!VL53L0X_performSingleRefCalibration(device_handle, 0x00)) {
		  // ESP_LOGI(" ","VL53L0X_performSingleRefCalibration2 false \n");
		   return false;
	   }

	   I2C_writeRegister8bit(device_handle, SYSTEM_SEQUENCE_CONFIG, 0xE8);

	   //ESP_LOGI(" ","VLX sensor init done! \n");
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
    uint32_t final_range_timeout_mclks =
    VL53L0X_timeoutMicrosecondsToMclks(final_range_timeout_us,
                                timeouts.final_range_vcsel_period_pclks);

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
  uint8_t sequence_config = (uint8_t)(I2C_readRegister16bit(device_handle, SYSTEM_SEQUENCE_CONFIG)>>8);

  enables->tcc          = (sequence_config >> 4) & 0x1;
  enables->dss          = (sequence_config >> 3) & 0x1;
  enables->msrc         = (sequence_config >> 2) & 0x1;
  enables->pre_range    = (sequence_config >> 6) & 0x1;
  enables->final_range  = (sequence_config >> 7) & 0x1;
}

void VL53L0X_getSequenceStepTimeouts(I2C_dev_handles device_handle, SequenceStepEnables const * enables, SequenceStepTimeouts * timeouts)
{
  timeouts->pre_range_vcsel_period_pclks = VL53L0X_getVcselPulsePeriod(device_handle, VcselPeriodPreRange);
  timeouts->msrc_dss_tcc_mclks = (uint8_t)(I2C_readRegister16bit(device_handle, MSRC_CONFIG_TIMEOUT_MACROP)>>8) + 1;
  timeouts->msrc_dss_tcc_us = VL53L0X_timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks,
                               timeouts->pre_range_vcsel_period_pclks);
  timeouts->pre_range_mclks = VL53L0X_decodeTimeout(I2C_readRegister16bit(device_handle, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
  timeouts->pre_range_us =VL53L0X_timeoutMclksToMicroseconds(timeouts->pre_range_mclks,
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
    return decodeVcselPeriod((uint8_t)(I2C_readRegister16bit(device_handle, PRE_RANGE_CONFIG_VCSEL_PERIOD)>>8));
  }
  else if (type == VcselPeriodFinalRange)
  {
    return decodeVcselPeriod((uint8_t)(I2C_readRegister16bit(device_handle, FINAL_RANGE_CONFIG_VCSEL_PERIOD)>>8));
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
  uint16_t reg2;
  startTimeout();
  do
  {
	  reg2 = I2C_readRegister16bit(device_handle, RESULT_INTERRUPT_STATUS);
	  //ESP_LOGI("","Single: %"PRIu16" ",reg2);
	  if (checkTimeoutExpired()) { return false; }
	  vTaskDelay(pdMS_TO_TICKS(500));
  } while(((uint8_t)reg2 & 0x07) == 0);

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
  //ESP_LOGI(" ", "AICI");
   //"Wait until start bit has been cleared"
  startTimeout();
  while (((uint8_t)(I2C_readRegister16bit(device_handle, SYSRANGE_START)>>8) & 0x01))
  {
    if (checkTimeoutExpired())
    {
      //did_timeout = true;
      return 65535;
    }
  }

  return VL53L0X_readRangeContinuousMillimeters(device_handle);
}

uint16_t VL53L0X_readRangeContinuousMillimeters(I2C_dev_handles device_handle)
{
  startTimeout();
  uint16_t range = 0;
  static bool start_continuous_back2back_sens1 = false;
  static bool start_continuous_back2back_sens2 = false;
  if((start_continuous_back2back_sens1 == false && device_handle == I2C_distance_sens1_dev_handle) ||
	 ((start_continuous_back2back_sens2 == false && device_handle == I2C_distance_sens2_dev_handle)))
  {
	  I2C_writeRegister8bit(device_handle, 0x80, 0x01);
	  I2C_writeRegister8bit(device_handle, 0xFF, 0x01);
	  I2C_writeRegister8bit(device_handle, 0x00, 0x00);
	  I2C_writeRegister8bit(device_handle, 0x91, stop_variable);
	  I2C_writeRegister8bit(device_handle, 0x00, 0x01);
	  I2C_writeRegister8bit(device_handle, 0xFF, 0x00);
	  I2C_writeRegister8bit(device_handle, 0x80, 0x00);
	  I2C_writeRegister8bit(device_handle, SYSRANGE_START, 0x02); // back to back mode = fast as possible
	  if(device_handle == I2C_distance_sens1_dev_handle)
	  start_continuous_back2back_sens1 = true;
	  else start_continuous_back2back_sens2 = true;
	  vTaskDelay(pdMS_TO_TICKS(20));
  }

  while (((uint8_t)(I2C_readRegister16bit(device_handle, RESULT_INTERRUPT_STATUS)>>8) & 0x07) == 0)
  {
    if (checkTimeoutExpired())
    {
      //did_timeout = true;
      return 0;
    }
  }
  // assumptions: Linearity Corrective Gain is 1000 (default);
  // fractional ranging is not enabled
  range = I2C_readRegister16bit(device_handle, RESULT_RANGE_STATUS + 10);
  I2C_writeRegister8bit(device_handle, SYSTEM_INTERRUPT_CLEAR, 0x01);


  return range;
}

void VL53L0X_SetInterruptThresholds(I2C_dev_handles device_handle,
                                             uint32_t ThresholdLow,
											 uint32_t ThresholdHigh) {
  uint16_t Threshold16;

  /* no dependency on DeviceMode for Ewok */
  /* Need to divide by 2 because the FW will apply a x2 */
  Threshold16 = (uint16_t)((ThresholdLow >> 17) & 0x00fff);
  I2C_writeRegister16bit(device_handle,  SYSTEM_THRESH_LOW, Threshold16);

  /* Need to divide by 2 because the FW will apply a x2 */
  Threshold16 = (uint16_t)((ThresholdHigh >> 17) & 0x00fff);
  I2C_writeRegister16bit(device_handle,  SYSTEM_THRESH_HIGH, Threshold16);
  //ESP_LOGI("", "Set treshold.");
}




void VL53L0X_setupI2CRegisters(I2C_dev_handles device_handle) {
    const uint8_t regs_vals[][2] = {
        {0xFF, 0x01}, {0x00, 0x00},
        {0xFF, 0x00}, {0x09, 0x00}, {0x10, 0x00}, {0x11, 0x00},
        {0x24, 0x01}, {0x25, 0xFF}, {0x75, 0x00},
        {0xFF, 0x01}, {0x4E, 0x2C}, {0x48, 0x00}, {0x30, 0x20},
        {0xFF, 0x00}, {0x30, 0x09}, {0x54, 0x00}, {0x31, 0x04}, {0x32, 0x03}, {0x40, 0x83}, {0x46, 0x25},
        {0x60, 0x00}, {0x27, 0x00}, {0x50, 0x06}, {0x51, 0x00}, {0x52, 0x96}, {0x56, 0x08}, {0x57, 0x30},
        {0x61, 0x00}, {0x62, 0x00}, {0x64, 0x00}, {0x65, 0x00}, {0x66, 0xA0},
        {0xFF, 0x01}, {0x22, 0x32}, {0x47, 0x14}, {0x49, 0xFF}, {0x4A, 0x00},
        {0xFF, 0x00}, {0x7A, 0x0A}, {0x7B, 0x00}, {0x78, 0x21},
        {0xFF, 0x01}, {0x23, 0x34}, {0x42, 0x00}, {0x44, 0xFF}, {0x45, 0x26}, {0x46, 0x05},
        {0x40, 0x40}, {0x0E, 0x06}, {0x20, 0x1A}, {0x43, 0x40},
        {0xFF, 0x00}, {0x34, 0x03}, {0x35, 0x44},
        {0xFF, 0x01}, {0x31, 0x04}, {0x4B, 0x09}, {0x4C, 0x05}, {0x4D, 0x04},
        {0xFF, 0x00}, {0x44, 0x00}, {0x45, 0x20}, {0x47, 0x08}, {0x48, 0x28}, {0x67, 0x00},
        {0x70, 0x04}, {0x71, 0x01}, {0x72, 0xFE}, {0x76, 0x00}, {0x77, 0x00},
        {0xFF, 0x01}, {0x0D, 0x01},
        {0xFF, 0x00}, {0x80, 0x01}, {0x01, 0xF8},
        {0xFF, 0x01}, {0x8E, 0x01}, {0x00, 0x01},
        {0xFF, 0x00}, {0x80, 0x00},
    };
    writeMultipleRegisters(device_handle, regs_vals, sizeof(regs_vals) / sizeof(regs_vals[0]));
}
