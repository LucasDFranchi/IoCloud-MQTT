#ifndef MAX6675_H
#define MAX6675_H

#include "esp_err.h"

/**
 * @file sntp_task.h
 * @brief Interface for SNTP time synchronization task for ESP32.
 *
 * This header defines the interface for initializing and managing the SNTP
 * task, which synchronizes the system time with an NTP server and signals
 * successful synchronization through an event group.
 */

/**
 * @brief Initializes the MAX6675 SPI communication.
 *
 * Configures the SPI bus and attaches the MAX6675 sensor as an SPI device.
 * This function allows the user to specify the MISO, CLK, and CS pins dynamically.
 *
 * @param miso_pin GPIO number for the SPI MISO pin.
 * @param clk_pin GPIO number for the SPI clock (SCLK) pin.
 * @param cs_pin GPIO number for the SPI chip select (CS) pin.
 *
 * @return esp_err_t ESP_OK on success, or an error code if initialization fails.
 */
esp_err_t max6675_initialize(int miso_pin, int clk_pin, int cs_pin);

/**
 * @brief Reads the temperature from the MAX6675 sensor.
 *
 * Performs an SPI transaction to retrieve the raw temperature data,
 * checks for errors, and converts the value to Celsius.
 *
 * @return float Temperature in Celsius (°C). Returns -1.0 on failure.
 */
float max6675_get_temperature(int gain, int offset);

#endif  // MAX6675_H
