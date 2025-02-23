#include "application_task.h"
#include "Driver/max6675.h"
#include "application_external_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "global_config.h"

static const int SPI_MISO_PIN = 19;
static const int SPI_CLK_PIN  = 18;
static const int SPI_CS_PIN   = 5;

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
static global_config_st *global_config = NULL;  ///< Global configuration structure.

static const char *TAG = "Application Task";  ///< Tag used for logging.

/**
 * @brief Attempts to fetch a new configuration from the queue.
 *
 * This function checks if there is a new configuration available in the
 * queue and retrieves it if present.
 *
 * @param command_config Pointer to store the fetched configuration.
 * @return true if a new configuration was retrieved, false otherwise.
 */
static void try_fetch_new_config(temperature_config_st *temperature_config) {
    if (temperature_config == NULL) {
        return;
    }

    BaseType_t is_data_in_queue =
        xQueueReceive(global_config->mqtt_topics[DATA_STRUCT_TEMPERATURE_CONFIG].queue,
                      temperature_config,
                      pdMS_TO_TICKS(100));

    if (is_data_in_queue == pdTRUE) {
        ESP_LOGI(TAG, "%s - New configuration received: Time Interval %ld",
                 __func__,
                 temperature_config->time_interval);
    }
}

/**
 * @brief Attempts to fetch a new temperature calibration from the queue.
 *
 * This function checks if a new calibration configuration is available in
 * the queue and retrieves it if present. If no new calibration is found
 * within the specified timeout, the function returns without updating the
 * provided structure.
 *
 * @param[out] temperature_calibration Pointer to the structure where the
 *                retrieved calibration values (gain and offset) will be stored.
 *                Must not be NULL.
 */
static void try_fetch_new_calibration(temperature_calibration_st *temperature_calibration) {
    if (temperature_calibration == NULL) {
        return;
    }

    BaseType_t is_data_in_queue =
        xQueueReceive(global_config->mqtt_topics[DATA_STRUCT_TEMPERATURE_CALIBRATION].queue,
                      temperature_calibration,
                      pdMS_TO_TICKS(100));

    if (is_data_in_queue == pdTRUE) {
        ESP_LOGI(TAG, "%s - New calibration received: Gain %ld, Offset %ld",
                 __func__,
                 temperature_calibration->gain,
                 temperature_calibration->offset);
    }
}

/**
 * @brief Initializes the application hardware and peripherals.
 *
 * This function is responsible for setting up the necessary hardware components,
 * If the initialization fails, the function logs an error and may enter a wait loop.
 *
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t application_task_initialize(void) {
    return max6675_initialize(SPI_MISO_PIN, SPI_CLK_PIN, SPI_CS_PIN);
}

/**
 * @brief Main application task loop.
 *
 * This task is responsible for continuously processing incoming commands and
 * managing responses based on the detected field conditions.
 *
 * @param pvParameters Pointer to the global configuration structure.
 */
void application_task_execute(void *pvParameters) {
    global_config = (global_config_st *)pvParameters;
    if ((application_task_initialize() != ESP_OK) || (global_config == NULL)) {
        ESP_LOGE(TAG, " %s - Failed to initialize application task", __func__);
        vTaskDelete(NULL);
    }
    temperature_config_st temperature_config = {
        .time_interval = 1000,
    };
    temperature_calibration_st temperature_calibration = {
        .gain   = 1,
        .offset = 0,
    };

    while (1) {
        try_fetch_new_config(&temperature_config);
        try_fetch_new_calibration(&temperature_calibration);

        temperature_response_st temperature_response = {
            .internal_temperature = 25,
            .humidity             = 50,
            .temperature_array    = 0,
        };

        temperature_response.temperature_array = max6675_get_temperature(temperature_calibration.gain,
                                                                         temperature_calibration.offset);

        BaseType_t queue_result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_TEMPERATURE_RESPONSE].queue,
                                             &temperature_response,
                                             pdMS_TO_TICKS(100));

        if (queue_result != pdPASS) {
            ESP_LOGW(TAG, " %s - Failed to send %s data to queue",
                     __func__,
                     global_config->mqtt_topics[DATA_STRUCT_TEMPERATURE_RESPONSE].topic);
        }

        vTaskDelay(pdMS_TO_TICKS(temperature_config.time_interval));
    }
}
