#include "application_task.h"
#include "global_config.h"
#include "http_server_task.h"
#include "mqtt_client_task.h"
#include "network_task.h"
#include "sntp_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st *global_config = NULL;
/*
 * @brief Initialize the Non-Volatile Storage (NVS) for the device.
 *
 * This function initializes the NVS (Non-Volatile Storage) module, which is used to store
 * configuration data and other persistent information. If the NVS initialization fails due
 * to no free pages or a new version of NVS being found, it will erase the existing NVS data
 * and reinitialize it.
 *
 * @return 
 * - ESP_OK on successful initialization.
 * - Other error codes from `nvs_flash_init()` in case of failure.
 */
esp_err_t initialize_nvs(void) {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result;
}

void app_main() {
    ESP_ERROR_CHECK(initialize_nvs());

    global_config.firmware_event_group = xEventGroupCreate();
    global_config.app_data_queue       = xQueueCreate(10, sizeof(temperature_data_st));

    xTaskCreate(
        network_task_execute,
        NETWORK_TASK_NAME,
        NETWORK_TASK_STACK_SIZE,
        (void *)&global_config,
        NETWORK_TASK_PRIORITY,
        NULL);

    xTaskCreate(
        http_server_task_execute,
        HTTP_SERVER_TASK_NAME,
        HTTP_SERVER_TASK_STACK_SIZE,
        (void *)&global_config,
        HTTP_SERVER_TASK_PRIORITY,
        NULL);

    xTaskCreate(
        application_task_execute,
        APPLICATION_TASK_NAME,
        APPLICATION_TASK_STACK_SIZE,
        (void *)&global_config,
        APPLICATION_TASK_PRIORITY,
        NULL);

    xTaskCreate(
        mqtt_client_task_execute,
        MQTT_CLIENT_TASK_NAME,
        MQTT_CLIENT_TASK_STACK_SIZE,
        (void *)&global_config,
        MQTT_CLIENT_TASK_PRIORITY,
        NULL);

    xTaskCreate(
        sntp_task_execute,
        SNTP_TASK_NAME,
        SNTP_TASK_STACK_SIZE,
        (void *)&global_config,
        SNTP_TASK_PRIORITY,
        NULL);
}