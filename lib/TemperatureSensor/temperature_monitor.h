#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/**
 * @brief External handle for the sensor data queue.
 *
 * This queue is used to store and retrieve temperature and humidity data
 * captured from the sensors. It acts as a communication mechanism between
 * tasks producing sensor data and tasks consuming it for processing or
 * transmission.
 */
extern QueueHandle_t sensor_data_queue;

/**
 * @brief Data structure to hold temperature and humidity information.
 *
 * This structure is designed to encapsulate environmental data read from a
 * temperature and humidity sensor. It contains fields for temperature and
 * relative humidity values.
 */
typedef struct temperature_data_t {
    float temperature; ///< Temperature reading from the sensor in degrees Celsius.
    float humidity;    ///< Humidity reading from the sensor in percentage (%).
} temperature_data_st;

/**
 * @file temperature_monitor.h
 * @brief Interface for managing the temperature sensor.
 *
 * This module provides functions for interacting with a temperature sensor.
 * It includes the main execution function for continuously monitoring the
 * sensor and performing necessary actions based on temperature readings.
 */

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
void temperature_monitor_execute(void *pvParameters);

#endif /* TEMPERATURE_MONITOR_H */
