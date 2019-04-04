#ifndef __I2C_H__
#define __I2C_H__

#include "esp_err.h"

esp_err_t i2c_init();
esp_err_t i2c_write(uint8_t DEV_ADDR, uint8_t reg_address, uint8_t *data, size_t data_len);
esp_err_t i2c_read(uint8_t DEV_ADDR, uint8_t reg_address, uint8_t *data, size_t data_len);
esp_err_t i2c_exit();


#endif