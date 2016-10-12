// author: Pawel Hryniszak
// bits and pieces stolen from many places...

#ifndef SI7021_H__
#define SI7021_H__

#include "app_twi.h"

#define SI7021_ADDR (0x40U)

// Measure Relative Humidity, Hold Master Mode
#define SI7021_REG_HUMIDITY_HM 0xE5
// Measure Relative Humidity, No Hold Master Mode
#define SI7021_REG_HUMIDITY_NA 0xF5
// Measure Temperature, Hold Master Mode
#define SI7021_REG_TEMPERATURE_HM 0xE3
// Measure Temperature, No Hold Master Mode
#define SI7021_REG_TEMPERATURE_NA 0xF3
// Read Temperature Value from Previous RH Measurement
#define SI7021_REG_TEMPERATURE_PREV 0xE0
// Reset
#define SI7021_REG_RESET 0xFE
// Write RH/T User Register 1
#define SI7021_REG_WR 0xE6
// Read RH/T User Register 1
#define SI7021_REG_RD 0xE7
// Write Heater Control Register
#define SI7021_REG_WR_HEATER 0x51
// Read Heater Control Register
#define SI7021_REG_RD_HEATER 0x11
// Read Electronic ID 1st Byte 0xFA 0x0F
// Read Electronic ID 2nd Byte 0xFC 0xC9
// Read Firmware Revision 0x84 0xB8

#define RH_CALC(n)		(((125 * (float) (n)) / 65536) - 6)
#define RH_CALC100(n)		(((12500 * n) / 65536) - 600)

#define TEMP_CALC(n)	(((175.72f * (n)) / 65536) - 46.85f) 
#define TEMP_CALC100(n)	(((17572 * n) / 65536) - 4685) 


#define SI7021_READ(p_reg_addr, p_buffer, byte_cnt)          \
  APP_TWI_WRITE(SI7021_ADDR, p_reg_addr, 1, APP_TWI_NO_STOP) \
  ,                                                          \
      APP_TWI_READ(SI7021_ADDR, p_buffer, byte_cnt, 0)

#define SI7021_READ_2(p_reg_addr, p_buffer, byte_cnt)        \
  APP_TWI_WRITE(SI7021_ADDR, p_reg_addr, 2, APP_TWI_NO_STOP) \
  ,                                                          \
      APP_TWI_READ(SI7021_ADDR, p_buffer, byte_cnt, 0)

// reset transaction
static uint8_t const SI7021_REG_RESET_CMD[] = {SI7021_REG_RESET, 0};
static app_twi_transfer_t const SI7021_RESET[] = {
    APP_TWI_WRITE(SI7021_ADDR, SI7021_REG_RESET_CMD, 1, 0)
}; 

// measurement storage
static uint8_t si7021_rh[3] = {0};        // 2+crc
static uint8_t si7021_temp[3] = {0};      // 2+crc
static uint8_t si7021_config[1] = {0}; 

// read config
static uint8_t const SI7021_REG_RD_CMD[] = {SI7021_REG_RD, 0};
static app_twi_transfer_t const SI7021_CONFIG[] = {
    SI7021_READ(SI7021_REG_RD_CMD, si7021_config, 1)
}; 

// trigger measure of rh and temp
static uint8_t const SI7021_REG_HUMIDITY_CMD[] = {SI7021_REG_HUMIDITY_NA, 0};
//static app_twi_transfer_t const SI7021_TRIG[] = {
//    APP_TWI_WRITE(SI7021_ADDR, SI7021_REG_HUMIDITY_CMD, 1, 0),
//};
 
// read relative humidity & tremperature transaction
//NOTE: must be used after a SI7021_TRIG, with a ~17 (max 22) ms delay
static uint8_t const SI7021_READ_TEMP_CMD[] = {SI7021_REG_TEMPERATURE_PREV, 0};
static uint8_t const SI7021_READ_ALL_CMD[] = {SI7021_REG_HUMIDITY_HM, 0};

static app_twi_transfer_t const SI7021_READ_ALL[] = {
    SI7021_READ(SI7021_READ_ALL_CMD, si7021_rh, 3),
    //APP_TWI_WRITE(SI7021_ADDR, SI7021_READ_ALL_CMD, 1, APP_TWI_NO_STOP),
    //APP_TWI_READ(SI7021_ADDR, si7021_rh, 3, 0),
    SI7021_READ(SI7021_READ_TEMP_CMD, si7021_temp, 3)
    // APP_TWI_WRITE(SI7021_ADDR, SI7021_READ_TEMP_CMD, 1, APP_TWI_NO_STOP),
    // APP_TWI_READ(SI7021_ADDR, si7021_temp, 3, 0),
}; 

float si7021getTemperature() {
    uint16_t temp = si7021_temp[1] | (uint16_t)si7021_temp[0] << 8;
    return TEMP_CALC(temp);
}

int16_t si7021getTemperature100() {
    uint16_t temp = si7021_temp[1] | (uint16_t)si7021_temp[0] << 8;
    return TEMP_CALC100(temp);
}

float si7021getHumidity() {
    uint16_t rh = si7021_rh[1] | (uint16_t)si7021_rh[0] << 8;
    return RH_CALC(rh);
}

int16_t si7021getHumidity100() {
    uint16_t rh = si7021_rh[1] | (uint16_t)si7021_rh[0] << 8;
    return RH_CALC100(rh);
}

#endif // SI7021_H__