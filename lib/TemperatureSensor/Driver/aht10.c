/**
 * @file aht10.c
 * @brief Driver for AHT10 temperature and humidity sensor.
 *
 * This file contains functions and configurations to interface with the AHT10
 * sensor over I2C. It supports initializing the sensor, triggering measurements,
 * and reading temperature and humidity data. The I2C master configuration is also
 * included for communication with the sensor.
 */

#include "aht10.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const gpio_num_t I2C_MASTER_SCL_IO     = GPIO_NUM_22;  ///< GPIO pin for I2C SCL
static const gpio_num_t I2C_MASTER_SDA_IO     = GPIO_NUM_21;  ///< GPIO pin for I2C SDA
static const i2c_port_t I2C_MASTER_NUM        = I2C_NUM_0;    ///< I2C port number
static const uint32_t I2C_MASTER_FREQ_HZ      = 100000;       ///< I2C clock frequency (100kHz standard)
static const size_t I2C_MASTER_TX_BUF_DISABLE = 0;            ///< Disable I2C TX buffer
static const size_t I2C_MASTER_RX_BUF_DISABLE = 0;            ///< Disable I2C RX buffer
static const uint8_t AHT10_SENSOR_ADDR        = 0x38;         ///< AHT10 sensor I2C address
static const uint8_t AHT10_CMD_INIT           = 0xE1;         ///< AHT10 initialization command
static const uint8_t AHT10_CMD_TRIGGER        = 0xAC;         ///< AHT10 trigger measurement command
static const uint8_t AHT10_CMD_TRIGGER_CONFIG = 0x33;         ///< Configuration byte for the trigger command
static const uint8_t AHT10_CMD_RESERVED       = 0x00;         ///< Reserved byte for the trigger command
static const char* TAG                        = "AHT10";      ///< Tag for logging.

/**
 * @brief Initializes the I2C master interface for communication with the AHT10 sensor.
 *
 * This function configures the I2C bus parameters, including the SDA and SCL pins,
 * clock speed, and pull-up resistors. It also installs the I2C driver for communication.
 *
 * @return ESP_OK on success, ESP_FAIL or other error codes if initialization fails.
 */
static esp_err_t aht10_i2c_master_init() {
    i2c_config_t config     = {};
    config.mode             = I2C_MODE_MASTER;
    config.sda_io_num       = I2C_MASTER_SDA_IO;
    config.scl_io_num       = I2C_MASTER_SCL_IO;
    config.sda_pullup_en    = GPIO_PULLUP_ENABLE;
    config.scl_pullup_en    = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = I2C_MASTER_FREQ_HZ;

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &config));
    return i2c_driver_install(I2C_MASTER_NUM, config.mode, I2C_MASTER_TX_BUF_DISABLE, I2C_MASTER_RX_BUF_DISABLE, 0);
}

/**
 * @brief Sends a single command byte to the AHT10 sensor over I2C.
 *
 * This function is used to send a command byte to the AHT10 sensor to trigger its operation.
 *
 * @param[in] cmd The command byte to send to the sensor.
 *
 * @return ESP_OK on success, or an error code if the I2C communication fails.
 */
static esp_err_t aht10_send_cmd(uint8_t cmd) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();

    i2c_master_start(handle);
    i2c_master_write_byte(handle, (AHT10_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, cmd, true);
    i2c_master_stop(handle);

    esp_err_t result = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(handle);

    return result;
}

/**
 * @brief Sends multiple command bytes to the AHT10 sensor over I2C.
 *
 * This function is used to send an array of command bytes to the AHT10 sensor to trigger
 * its operation.
 *
 * @param[in] cmds Pointer to an array of command bytes to send to the sensor.
 * @param[in] cmd_size The size of the command array.
 *
 * @return ESP_OK on success, or an error code if the I2C communication fails.
 */
static esp_err_t aht10_send_cmds(uint8_t* cmds, uint8_t cmd_size) {
    esp_err_t result        = ESP_FAIL;
    i2c_cmd_handle_t handle = i2c_cmd_link_create();

    do {
        if (cmd_size == 0) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }

        if (cmds == NULL) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        i2c_master_start(handle);
        i2c_master_write_byte(handle, (AHT10_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(handle, cmds, cmd_size, true);
        i2c_master_stop(handle);

        result = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(handle);
    } while (0);

    return result;
}


/**
 * @brief Reads data from the AHT10 sensor over I2C.
 *
 * This function reads a specified number of bytes from the AHT10 sensor, which may contain
 * temperature and humidity data or other sensor information.
 *
 * @param[out] data Pointer to a buffer where the sensor data will be stored.
 * @param[in] data_size The number of bytes to read from the sensor.
 *
 * @return ESP_OK on success, or an error code if the I2C communication fails.
 */
static esp_err_t aht10_read_data(uint8_t* data, uint8_t data_size) {
    esp_err_t result = ESP_FAIL;

    do {
        if (data == NULL) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        if (data_size == 0) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }

        i2c_cmd_handle_t handle = i2c_cmd_link_create();
        i2c_master_start(handle);
        i2c_master_write_byte(handle, (AHT10_SENSOR_ADDR << 1) | I2C_MASTER_READ, true);
        i2c_master_read(handle, data, data_size, I2C_MASTER_LAST_NACK);
        i2c_master_stop(handle);
        result = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(handle);

    } while (0);

    return result;
}

/**
 * @brief Initializes the AHT10 sensor by sending the initialization command.
 *
 * This function triggers the initialization process for the AHT10 sensor by sending
 * the initialization command over I2C.
 *
 * @return ESP_OK on success, ESP_FAIL or other error codes if initialization fails.
 */
esp_err_t aht10_init() {
    esp_err_t result = ESP_FAIL;

    result = ESP_ERROR_CHECK_WITHOUT_ABORT(aht10_i2c_master_init());

    ESP_LOGI(TAG, "Initializing AHT10 sensor...");
    result = ESP_ERROR_CHECK_WITHOUT_ABORT(aht10_send_cmd(AHT10_CMD_INIT));

    return result;
}

/**
 * @brief Retrieves the temperature and humidity data from the AHT10 sensor.
 *
 * This function sends the trigger command to the AHT10 sensor, waits for the measurement
 * to complete, and reads the temperature and humidity data.
 *
 * @param[out] aht10_data Pointer to a structure where the retrieved sensor data will be stored.
 *
 * @return ESP_OK on success, or an error code if reading the data fails.
 */
esp_err_t aht10_get_temperature_humidity(aht10_data_st* aht10_data) {
    esp_err_t result = ESP_FAIL;
    uint8_t cmds[]   = {AHT10_CMD_TRIGGER, AHT10_CMD_TRIGGER_CONFIG, AHT10_CMD_RESERVED};
    uint8_t data[6]  = {0};

    do {
        if (aht10_data == NULL) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        result = ESP_ERROR_CHECK_WITHOUT_ABORT(aht10_send_cmds(cmds, sizeof(cmds)));

        vTaskDelay(pdMS_TO_TICKS(100));

        if (result != ESP_OK) {
            break;
        }
        result = ESP_ERROR_CHECK_WITHOUT_ABORT(aht10_read_data(data, sizeof(data)));

        if (result != ESP_OK) {
            break;
        }

        aht10_data->raw_humidity    = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
        aht10_data->raw_temperature = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    } while (0);

    return result;
}
