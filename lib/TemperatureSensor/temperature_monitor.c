#include "temperature_monitor.h"
#include "Driver/aht10.h"
#include "esp_log.h"

#include "esp_err.h"

/**
 * @file temperature_monitor.c
 * @brief Temperature monitoring and logging for temperature and humidity data.
 *
 * This module interfaces with the AHT10 temperature and humidity sensor,
 * reads the data, and logs the temperature and humidity values periodically.
 * It provides functions to initialize the sensor, execute continuous readings,
 * and log the results.
 */

static aht10_data_st aht10_data             = {0};                    ///< Structure to hold the temperature and humidity data.
static temperature_data_st temperature_data = {0};                    ///< Structure to hold the temperature and humidity data for external use.
static const char* TAG                      = "Temperature Monitor";  ///< Tag used for logging.

QueueHandle_t sensor_data_queue = NULL;

/**
 * @brief Initializes the temperature sensor.
 *
 * This function calls the `aht10_init` function to initialize the AHT10
 * temperature and humidity sensor. If initialization fails, the task
 * will be deleted.
 *
 * @return ESP_OK on successful initialization, ESP_FAIL on failure.
 */
static esp_err_t temperature_sensor_initialize(void) {
    sensor_data_queue = xQueueCreate(100, sizeof(temperature_data_st));
    return aht10_init();
}

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
void temperature_monitor_execute(void* pvParameters) {
    if (temperature_sensor_initialize() != ESP_OK) {
        vTaskDelete(NULL);
    }

    while (1) {
        aht10_get_temperature_humidity(&aht10_data);

        temperature_data.humidity    = ((float)aht10_data.raw_humidity / 1048576.0) * 100.0;
        temperature_data.temperature = ((float)aht10_data.raw_temperature / 1048576.0) * 200.0 - 50.0;

        if (xQueueSend(sensor_data_queue, &temperature_data, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW(TAG, "Failed to send data to queue");
        }

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
