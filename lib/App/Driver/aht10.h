#ifndef AHT10_H
#define AHT10_H

#include "esp_err.h"

/**
 * @file aht10.h
 * @brief Interface for interacting with the AHT10 temperature and humidity sensor.
 *
 * This module provides functions to initialize the AHT10 sensor and retrieve
 * temperature and humidity data. It uses I2C communication to interface with
 * the sensor.
 */

/**
 * @brief Struct representing raw temperature and humidity data from the AHT10 sensor.
 *
 * This structure stores the raw sensor data for temperature and humidity readings,
 * which can be processed or converted to actual values.
 */
typedef struct aht10_data_t {
    uint32_t raw_temperature; ///< Raw temperature data from the AHT10 sensor.
    uint32_t raw_humidity;    ///< Raw humidity data from the AHT10 sensor.
} aht10_data_st;

/**
 * @brief Initializes the AHT10 sensor.
 *
 * This function initializes the AHT10 sensor by configuring the necessary I2C
 * communication and sending the required initialization commands to the sensor.
 *
 * @return ESP_OK on success, or an error code if the initialization fails.
 */
esp_err_t aht10_init();

/**
 * @brief Retrieves temperature and humidity data from the AHT10 sensor.
 *
 * This function triggers a measurement on the AHT10 sensor and retrieves the
 * raw temperature and humidity data. The retrieved data is stored in the
 * provided `aht10_data` structure.
 *
 * @param[out] aht10_data Pointer to a structure where the retrieved sensor data will be stored.
 *
 * @return ESP_OK on success, or an error code if the data retrieval fails.
 */
esp_err_t aht10_get_temperature_humidity(aht10_data_st* aht10_data);

#endif  // AHT10_H
