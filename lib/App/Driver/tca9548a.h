#pragma once

#include "driver/i2c.h"
#include "esp_log.h"

/**
 * @brief TCA9548A device descriptor
 */
typedef struct {
    i2c_port_t port_num;       /**< I2C port number */
    uint8_t dev_addr;    /**< I2C device address */
} tca9548a_t;

/**
 * @brief Initialize the TCA9548A multiplexer
 *
 * @param[out] dev Pointer to device descriptor
 * @param[in] sda_io_num GPIO number for I2C SDA
 * @param[in] scl_io_num GPIO number for I2C SCL
 * @param[in] port_num I2C port number
 * @param[in] dev_addr I2C device address of TCA9548A
 * @return
 *      - ESP_OK on success
 *      - ESP_FAIL or other error code otherwise
 */
esp_err_t tca9548a_initialize(const tca9548a_t *dev, int sda_io_num, int scl_io_num);

/**
 * @brief Select a single channel on TCA9548A
 *
 * This disables all other channels and enables only the selected one.
 *
 * @param[in] dev Pointer to device descriptor
 * @param[in] channel Channel number to enable (0-7)
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if channel is out of range
 *      - ESP_FAIL or other error code otherwise
 */
esp_err_t tca9548a_set_channel(const tca9548a_t *dev, uint8_t channel);
