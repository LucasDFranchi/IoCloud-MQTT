/**
 * @file TCA9548A.h
 * @brief Driver interface for the TCA9548A I2C multiplexer using ESP-IDF.
 *
 * This header defines the API for initializing and controlling the TCA9548A,
 * an 8-channel I2C switch that allows multiple devices with the same address
 * to coexist on a single bus by routing communication to one channel at a time.
 *
 * It includes structures for device configuration and functions for initialization
 * and channel selection. Designed for use with the ESP-IDF framework.
 *
 * @author Lucas D. Franchi
 * @license Apache License 2.0
 */
#pragma once

#include "driver/i2c.h"
#include "esp_log.h"

/**
 * @brief TCA9548A I2C multiplexer configuration structure.
 */
typedef struct {
    uint8_t dev_addr; /**< 7-bit I2C device address of the TCA9548A */
} tca9548a_config_st;

/**
 * @brief Initialize the TCA9548A multiplexer.
 *
 * Prepares the TCA9548A for use by setting up its device address.
 * This function may be used to validate communication or configure
 * software structures prior to using the multiplexer.
 *
 * @param[in] dev Pointer to the TCA9548A configuration structure.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the pointer is NULL
 *      - ESP_FAIL if initialization fails
 */
esp_err_t tca9548a_initialize(const tca9548a_config_st *dev);

/**
 * @brief Enable a single channel on the TCA9548A multiplexer.
 *
 * This function disables all other channels and enables only the one
 * specified by the user. Useful for selecting which I2C device is active
 * on a shared bus.
 *
 * @param[in] channel Channel number to enable (valid range: 0 to 7).
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the channel number is out of range
 *      - ESP_FAIL if communication with the TCA9548A fails
 */
esp_err_t tca9548a_set_channel(uint8_t channel);
