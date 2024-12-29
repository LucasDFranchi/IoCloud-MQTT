#ifndef APPLICATION_TASK_H
#define APPLICATION_TASK_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/**
 * @file application_task.h
 * @brief Interface for managing the temperature sensor.
 *
 * This module provides functions for interacting with a temperature sensor.
 * It includes the main execution function for continuously monitoring the
 * sensor and performing necessary actions based on temperature readings.
 */

/**
 * @brief Data structure to hold temperature and humidity information.
 *
 * This structure is designed to encapsulate environmental data read from a
 * temperature and humidity sensor. It contains fields for temperature and
 * relative humidity values.
 */
typedef struct temperature_data_t {
    float temperature;  ///< Temperature reading from the sensor in degrees Celsius.
    float humidity;     ///< Humidity reading from the sensor in percentage (%).
} temperature_data_st;

/**
 * @brief Main execution function for the temperature monitor.
 *
 * This function is responsible for executing the temperature monitor's tasks.
 * It continuously monitors the sensor and performs any required actions
 * such as reading the sensor data, logging the results, or triggering
 * events based on specific conditions.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void application_task_execute(void *pvParameters);

#endif /* APPLICATION_TASK_H */
