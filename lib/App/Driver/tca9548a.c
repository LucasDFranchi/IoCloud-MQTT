/**
 * @file TCA9548A.c
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
#include "tca9548a.h"

static const char *TAG     = "TCA9548A";  ///< Logging tag for ESP-IDF logging
static uint8_t dev_address = 0;           ///< Current I2C address of the TCA9548A
/**
 * @brief Send a single command byte to the TCA9548A device over I2C.
 *
 * This helper function constructs and sends an I2C write transaction to the
 * configured TCA9548A address. The byte typically represents the channel
 * selection bitmask.
 *
 * @param[in] data Byte to send to the device (e.g., 0x01 for channel 0).
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_FAIL or other I2C communication error
 */
static esp_err_t i2c_send_one_byte(uint8_t data) {
    esp_err_t ret_err = ESP_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret_err += i2c_master_start(cmd);

    ret_err += i2c_master_write_byte(cmd, (dev_address << 1) | I2C_MASTER_WRITE, true);
    ret_err += i2c_master_write_byte(cmd, data, true);
    ret_err += i2c_master_stop(cmd);

    ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Initialize the TCA9548A device descriptor.
 *
 * Stores the device I2C address for future communication.
 * Note: This function assumes the I2C driver is already initialized externally.
 *
 * @param[in] dev Pointer to TCA9548A configuration structure.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the input pointer is NULL
 */
esp_err_t tca9548a_initialize(const tca9548a_config_st *dev) {
    if (!dev) {
        ESP_LOGE(TAG, "Device descriptor is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    dev_address = dev->dev_addr;

    return ESP_OK;
}

/**
 * @brief Enable a single channel on the TCA9548A multiplexer.
 *
 * Sends a command to activate one of the 8 downstream channels (0–7).
 * Only the selected channel will be active; all others are disabled.
 *
 * @param[in] channel Channel number to activate (0 to 7).
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the channel number is out of range
 *      - ESP_FAIL if I2C transmission fails
 */
esp_err_t tca9548a_set_channel(uint8_t channel) {
    if (channel > 7) {
        ESP_LOGE(TAG, "Invalid channel: %d. Channel must be between 0 and 7.", channel);
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t data = 1 << channel;
    return i2c_send_one_byte(data);
}
