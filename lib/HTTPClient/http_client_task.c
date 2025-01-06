#include "esp_log.h"

#include "global_config.h"
#include "http_client_task.h"
#include "network_task.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st* global_config = NULL;

static const char* TAG            = "HTTP Client Task"; /**< Logging tag for HTTPClientProcess class. */
static bool is_network_connected  = false;


/**
 * @brief Starts the HTTP client and registers URI handlers.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t start_http_client(void) {
    esp_err_t result = ESP_OK;
    // httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // esp_err_t result      = httpd_start(&http_client, &config);
    // if (result == ESP_OK) {
    //     ESP_LOGI(TAG, "HTTP client started successfully");
    //     initialize_request_list();
    //     is_client_connected = true;
    // } else {
    //     ESP_LOGE(TAG, "Failed to start HTTP client: %s", esp_err_to_name(result));
    // }
    return result;
}

/**
 * @brief Stops the HTTP client.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
void stop_http_client(void) {
    // if (http_client) {
    //     httpd_stop(http_client);
    //     http_client         = NULL;
    //     is_client_connected = false;
    //     ESP_LOGI(TAG, "HTTP client stopped");
    // }
}

/**
 * @brief Initializes the HTTPManager by configuring client parameters and initializing request handlers.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t http_client_task_initialize(void) {
    esp_err_t result = ESP_OK;

    return result;
}

/**
 * @brief Main execution function for the HTTP client.
 *
 * This function initializes and starts the HTTP client, enabling the ESP32 to
 * handle incoming web requests. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void http_client_task_execute(void* pvParameters) {
    global_config = (global_config_st*)pvParameters;
    if ((http_client_task_initialize() != ESP_OK) ||
        (global_config == NULL) ||
        (global_config->firmware_event_group == NULL)) {
        ESP_LOGE(TAG, "Failed to initialize HTTP Client task");
        vTaskDelete(NULL);
    }

    while (1) {
        EventBits_t firmware_event_bits = xEventGroupWaitBits(global_config->firmware_event_group,
                                                              WIFI_CONNECTED_STA,
                                                              pdFALSE,
                                                              pdFALSE,
                                                              pdMS_TO_TICKS(100));

        if (is_network_connected) {
            if ((firmware_event_bits & WIFI_CONNECTED_STA) == 0) {
                stop_http_client();
            }
        } else {
            if (firmware_event_bits & WIFI_CONNECTED_STA) {
                start_http_client();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(HTTP_CLIENT_TASK_DELAY));
    }
}
