#include "application_task.h"
#include "Driver/aht10.h"
#include "esp_log.h"
#include "global_config.h"

#include "esp_err.h"

/**
 * @file Application.c
 * @brief Temperature monitoring and logging for temperature and humidity data.
 *
 * This module interfaces with the AHT10 temperature and humidity sensor,
 * reads the data, and logs the temperature and humidity values periodically.
 * It provides functions to initialize the sensor, execute continuous readings,
 * and log the results.
 */

/**
 * @brief Pointer to the global configuration structure.
 * 
 * This variable is used to synchronize and manage all FreeRTOS events and queues 
 * across the system. It provides a centralized configuration and state management 
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st* global_config = NULL;  ///< Global configuration structure.

static aht10_data_st aht10_data             = {0};                 ///< Structure to hold the temperature and humidity data.
static temperature_data_st temperature_data = {0};                 ///< Structure to hold the temperature and humidity data for external use.
static const char* TAG                      = "Application Task";  ///< Tag used for logging.

/**
 * @brief Initializes the temperature monitor.
 *
 * This function calls the `aht10_init` function to initialize the AHT10
 * temperature and humidity sensor. If initialization fails, the task
 * will be deleted.
 *
 * @return ESP_OK on successful initialization, ESP_FAIL on failure.
 */
static esp_err_t
application_task_initialize(void) {
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
void application_task_execute(void* pvParameters) {
    global_config = (global_config_st*)pvParameters;
    if ((application_task_initialize() != ESP_OK) ||
        (global_config == NULL) ||
        (global_config->app_data_queue == NULL)) {
        ESP_LOGE(TAG, "Failed to initialize application task");
        vTaskDelete(NULL);
    }

    while (1) {
        aht10_get_temperature_humidity(&aht10_data);

        temperature_data.humidity    = ((float)aht10_data.raw_humidity / 1048576.0) * 100.0;
        temperature_data.temperature = ((float)aht10_data.raw_temperature / 1048576.0) * 200.0 - 50.0;

        if (xQueueSend(global_config->app_data_queue, &temperature_data, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW(TAG, "Failed to send data to queue");
        }

        vTaskDelay(pdMS_TO_TICKS(APPLICATION_TASK_DELAY));
    }
}
