#include "application_task.h"
#include "global_config.h"
#include "http_server_task.h"
#include "mqtt_client_task.h"
#include "network_task.h"
#include "sntp_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st global_config = {0};

void app_main() {
    ESP_ERROR_CHECK(global_config_initialize(&global_config));
    ESP_ERROR_CHECK(mqtt_topic_initialize(&global_config, "temperature", 1));
    ESP_ERROR_CHECK(mqtt_topic_initialize(&global_config, "humidity", 1));

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