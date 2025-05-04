/* 
 * @file ADS1115.c
 * @brief ADS1115 ADC driver implementation using ESP-IDF I2C APIs.
 * 
 * This file provides functions for configuring and using the ADS1115 ADC 
 * over I2C, including register-level control and data acquisition.
 * 
 * @license Apache License 2.0
 * @author LucasD.Franchi@gmail.com
 */
#include <driver/i2c.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

#include "ADS1115.h"

#define ADS_RW_BUFF_SIZE 2  // Size of the read/write buffer

static const char *TAG = "ADS1115";

static register_config_ut register_config;
static uint8_t write_buffer[ADS_RW_BUFF_SIZE] = {0};
static uint8_t read_buffer[ADS_RW_BUFF_SIZE]  = {0};
static ads1115_config_st _ads1115_config      = {0};

/**
 * @brief Perform an I2C write to the specified device register.
 * 
 * @param dev_adr I2C device address
 * @param w_adr Register address to write to
 * @param w_len Number of bytes to write
 * @param buff Pointer to the buffer containing data to write
 * 
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t i2c_handle_write(uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff) {
    esp_err_t ret_err = ESP_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret_err += i2c_master_start(cmd);

    ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    ret_err += i2c_master_write_byte(cmd, w_adr, true);
    ret_err += i2c_master_write(cmd, buff, w_len, true);
    ret_err += i2c_master_stop(cmd);

    ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Perform an I2C read from the specified device register.
 * 
 * @param dev_adr I2C device address
 * @param r_adr Register address to read from
 * @param r_len Number of bytes to read
 * @param buff Pointer to the buffer to store read data
 * 
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t i2c_handle_read(uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff) {
    memset(buff, 0, ADS_RW_BUFF_SIZE);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret_err    = ESP_OK;

    ret_err += i2c_master_start(cmd);

    ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    ret_err += i2c_master_write_byte(cmd, r_adr, true);
    ret_err += i2c_master_start(cmd);
    ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_READ, true);

    if (r_len > 1)
        ret_err += i2c_master_read(cmd, buff, r_len - 1, I2C_MASTER_ACK);
    ret_err += i2c_master_read_byte(cmd, buff + r_len - 1, I2C_MASTER_NACK);
    ret_err += i2c_master_stop(cmd);

    ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Write a 16-bit value to the specified ADS1115 register.
 * 
 * @param value 16-bit value to write
 * @param register_address Register address to write to
 * 
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t write_register(uint16_t value, uint16_t register_address) {
    write_buffer[0] = (uint8_t)(value >> 8) & 0xFF;
    write_buffer[1] = (uint8_t)value & 0xFF;

    return i2c_handle_write(_ads1115_config.dev_addr, register_address, sizeof(write_buffer), write_buffer);
}


/**
 * @brief Read a 16-bit value from the specified ADS1115 register.
 * 
 * @param register_address Register address to read from
 * 
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t read_register(uint8_t register_address) {
    return i2c_handle_read(_ads1115_config.dev_addr, register_address, sizeof(read_buffer), read_buffer);
}

/**
 * @brief Set the comparator queue configuration.
 * @param que Comparator queue mode (use @ref comparator_queue_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_que(comparator_queue_et que) {
    if (que > 3)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.comp_que = que;
    return ESP_OK;
}

/**
 * @brief Set the comparator latching mode.
 * @param lat Latching mode (use @ref comparator_latching_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_lat(comparator_latching_t lat) {
    if (lat > 1)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.comp_lat = lat;
    return ESP_OK;
}

/**
 * @brief Set the comparator polarity.
 * @param pol Polarity (use @ref comparator_polarity_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_pol(comparator_polarity_et pol) {
    if (pol > 1)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.comp_pol = pol;
    return ESP_OK;
}

/**
 * @brief Set the comparator mode.
 * @param mode Mode (use @ref comparator_mode_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_mode(comparator_mode_t mode) {
    if (mode > 1)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.comp_mode = mode;
    return ESP_OK;
}

/**
 * @brief Set the ADC operating mode.
 * @param mode Operating mode (use @ref operating_mode_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_mode(operating_mode_et mode) {
    if (mode > 1)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.mode = mode;
    return ESP_OK;
}

/**
 * @brief Set the ADC data rate.
 * @param rate Data rate (use @ref data_rate_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_data_rate(data_rate_et rate) {
    if (rate > 7)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.dr = rate;
    return ESP_OK;
}

/**
 * @brief Set the programmable gain amplifier (PGA) value.
 * @param pga PGA setting (use @ref pga_gain_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_pga(pga_gain_et pga) {
    if (pga > 7)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.pga = pga;
    return ESP_OK;
}

/**
 * @brief Set the input multiplexer configuration.
 * @param mux Mux setting (use @ref mux_config_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_mux(mux_config_t mux) {
    if (mux > 7)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.mux = mux;
    return ESP_OK;
}

/**
 * @brief Set the operational status (start conversion).
 * @param os OS status (use @ref os_status_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_os(os_status_t os) {
    if (os > 1)
        return ESP_ERR_INVALID_ARG;
    register_config.bits.os = os;
    return ESP_OK;
}

/**
 * @brief Write the current configuration to the ADS1115 device.
 * 
 * Applies the internal configuration stored in `register_config` 
 * to the ADS1115 config register via I2C.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ADS1115_update(void) {
    return write_register(register_config.value, REG_ADDR_CONFIG);
}

/**
 * @brief Initialize the ADS1115 device with provided configuration.
 * 
 * Copies the device address and register configuration from the input structure, 
 * applies settings, and writes to the device.
 * 
 * @param dev Pointer to ADS1115 configuration structure
 * 
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if input is NULL
 */
esp_err_t ADS1115_initialize(ads1115_config_st *dev) {
    if (!dev) {
        ESP_LOGE(TAG, "Device descriptor is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    _ads1115_config.dev_addr      = dev->dev_addr;
    _ads1115_config.reg_cfg.value = dev->reg_cfg.value;

    ADS1115_set_comp_que(dev->reg_cfg.bits.comp_que);    ///< Set initial comparator queue
    ADS1115_set_comp_lat(dev->reg_cfg.bits.comp_lat);    ///< Set initial comparator latching
    ADS1115_set_comp_pol(dev->reg_cfg.bits.comp_pol);    ///< Set initial comparator polarity
    ADS1115_set_comp_mode(dev->reg_cfg.bits.comp_mode);  ///< Set initial comparator mode
    ADS1115_set_mode(dev->reg_cfg.bits.mode);            ///< Set initial operating mode
    ADS1115_set_data_rate(dev->reg_cfg.bits.dr);         ///< Set initial data rate
    ADS1115_set_pga(dev->reg_cfg.bits.pga);              ///< Set initial programmable gain amplifier
    ADS1115_set_mux(dev->reg_cfg.bits.mux);              ///< Set initial input multiplexer
    ADS1115_set_os(dev->reg_cfg.bits.os);                ///< Set initial operational status

    return ADS1115_update();
}

/**
 * @brief Read the current raw conversion result from the ADS1115.
 * 
 * Initiates an I2C read from the conversion register.
 * 
 * @return 16-bit raw ADC value
 */
uint16_t ADS1115_get_raw_value() {
    read_register(REG_ADDR_CONVERSION);
    return (uint16_t)(((read_buffer[0] << 8) & 0xFF00) | read_buffer[1]);
}

/**
 * @brief Check whether a conversion is currently complete.
 * 
 * Reads the operational status (OS) bit in the config register to determine
 * if the conversion is complete.
 * 
 * @return true if conversion is complete, false otherwise
 */
bool ADS1115_get_conversion_state()
{
    read_register(REG_ADDR_CONFIG);
    return (read_buffer[0] & 0x80) ? true : false;
}
