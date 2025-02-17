/**
 * @file watchdog_task.c
 * @brief Implementation of a Task Watchdog Timer (TWDT) task for ESP32.
 *
 * This file contains the implementation of a watchdog task that ensures
 * system stability by monitoring task execution. It uses the ESP-IDF Task
 * Watchdog Timer (TWDT) to detect unresponsive tasks and trigger a system reset
 * if necessary. The watchdog task periodically resets the watchdog timer to
 * prevent unwanted resets while the system is functioning correctly.
 */
#include "watchdog_task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "global_config.h"

static const char *TAG                          = "Watchdog Task";  ///< Tag for logging
static const uint32_t WATCHDOG_TIMEOUT_MILI_SEC = 15000;            ///< Watchdog timeout in miliseconds

/**
 * @brief Initialize the Task Watchdog Timer (TWDT).
 *
 * This function configures and initializes the TWDT with a specified timeout.
 * It enables panic handling, meaning that if a registered task fails to reset
 * the watchdog within the timeout period, the system will trigger a reset.
 *
 * @return
 *  - ESP_OK: Watchdog initialized successfully.
 *  - ESP_ERR_INVALID_STATE: Watchdog already initialized.
 *  - Other error codes indicate failure.
 */
static esp_err_t watchdog_task_initialize(void) {
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms     = WATCHDOG_TIMEOUT_MILI_SEC,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic  = true};

    esp_err_t result = ESP_FAIL;
    if (!CONFIG_ESP_TASK_WDT_INIT) {
        result = esp_task_wdt_init(&twdt_config);
    } else {
        result = esp_task_wdt_reconfigure(&twdt_config);
    }

    ESP_ERROR_CHECK(result);

    return result;
}

/**
 * @brief Task to monitor system stability using the Watchdog Timer.
 *
 * This task registers itself with the TWDT and continuously resets the watchdog
 * timer to indicate normal operation. If the task becomes unresponsive and fails
 * to reset the watchdog within the specified timeout, the system will reset.
 *
 * @param pvParameters Unused parameter (can be NULL).
 */
void watchdog_task_execute(void *pvParameters) {
    ESP_LOGI(TAG, "Starting Watchdog task execution...");

    if (watchdog_task_initialize() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Watchdog task");
        vTaskDelete(NULL);
    }

    esp_task_wdt_add(NULL);

    while (1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_TASK_DELAY));
    }
}