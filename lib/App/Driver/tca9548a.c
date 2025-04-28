#include "tca9548a.h"

static const char *TAG = "TCA9548A";
static const uint32_t I2C_MASTER_FREQ_HZ = 100000; // 100kHz

/**
 * @brief Initialize the I2C master interface for TCA9548A
 *
 * @param[inout] dev Pointer to device descriptor
 * @param[in] sda_io_num GPIO number for I2C SDA
 * @param[in] scl_io_num GPIO number for I2C SCL
 * @return
 *      - ESP_OK on success
 *      - ESP_FAIL or other error code otherwise
 */
static esp_err_t i2c_master_init(const tca9548a_t *dev, int sda_io_num, int scl_io_num) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_io_num,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_io_num,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(dev->port_num, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C parameters: %s", esp_err_to_name(ret));
        return ret;
    }

    // Install driver only if not already installed
    ret = i2c_driver_install(dev->port_num, conf.mode, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Send one byte to the TCA9548A device
 *
 * @param[in] dev Pointer to device descriptor
 * @param[in] data Byte to send
 * @return
 *      - ESP_OK on success
 *      - ESP_FAIL or other error code otherwise
 */
static esp_err_t i2c_send_one_byte(const tca9548a_t *dev, uint8_t data) {
    return i2c_master_write_to_device(dev->port_num, dev->dev_addr, &data, 1, pdMS_TO_TICKS(1000));
}

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
esp_err_t tca9548a_initialize(const tca9548a_t *dev, int sda_io_num, int scl_io_num) {
    return i2c_master_init(dev, sda_io_num, scl_io_num);
}

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
esp_err_t tca9548a_set_channel(const tca9548a_t *dev, uint8_t channel) {
    if (channel > 7) {
        ESP_LOGE(TAG, "Invalid channel: %d. Channel must be between 0 and 7.", channel);
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t data = 1 << channel;
    return i2c_send_one_byte(dev, data);
}
