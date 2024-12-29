#include "mqtt_client_task.h"
#include "application_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "global_config.h"
#include "mqtt_client.h"
#include "network_task.h"
#include "utils.h"
/**
 * @file
 * @brief MQTT client task implementation for managing MQTT connection and publishing sensor data.
 */
/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st* global_config = NULL;

static const char* TAG                      = "MQTT Task";  ///< Log tag for MQTT task.
static esp_mqtt_client_handle_t mqtt_client = {0};          ///< MQTT client handle.
static bool is_mqtt_connected               = false;        ///< MQTT connection status.
char unique_id[13]                          = {0};          ///< Device unique ID (12 chars + null terminator).

/**
 * @brief Handles MQTT events triggered by the client.
 *
 * This callback is responsible for managing MQTT events such as connection,
 * disconnection, incoming data, and errors.
 *
 * @param[in] arg        User-defined argument (not used).
 * @param[in] base       Event base, typically MQTT events.
 * @param[in] event_id   Specific MQTT event ID.
 * @param[in] event_data Event data containing details about the event.
 */
static void mqtt_event_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            is_mqtt_connected = true;
            esp_mqtt_client_subscribe(mqtt_client, "/titanium/timestamp", 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            is_mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA: Topic=%.*s, Data=%.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGD(TAG, "MQTT event: %d", event->event_id);
            break;
    }
}

/**
 * @brief Initializes the MQTT client and its configuration.
 *
 * This function sets up the MQTT client, configures its parameters, and
 * registers the event handler callback.
 */
static esp_err_t mqtt_client_task_initialize(void) {
    esp_mqtt_client_config_t mqtt_cfg = {0};
    esp_err_t result                  = ESP_OK;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    result += esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    result += esp_mqtt_client_set_uri(mqtt_client, "mqtt://mqtt.eclipseprojects.io");
    result += esp_mqtt_set_config(mqtt_client, &mqtt_cfg);

    get_unique_id(unique_id, sizeof(unique_id));

    return result;
}

/**
 * @brief Starts the MQTT client if it is initialized.
 *
 * This function starts the MQTT client and logs the status of the operation.
 */
static void start_mqtt_client(void) {
    if (mqtt_client) {
        esp_mqtt_client_start(mqtt_client);
        ESP_LOGI(TAG, "MQTT client started");
    } else {
        ESP_LOGE(TAG, "MQTT client not initialized");
    }
}

/**
 * @brief Stops the MQTT client if it is running.
 *
 * This function stops the MQTT client and logs the status of the operation.
 */
static void stop_mqtt_client(void) {
    if (mqtt_client) {
        esp_mqtt_client_stop(mqtt_client);
        ESP_LOGI(TAG, "MQTT client stopped");
    }
}

/**
 * @brief Publish the temperature data to the MQTT broker.
 *
 * This function formats the temperature value as a JSON string with a timestamp
 * and publishes it to the MQTT topic "/titanium/1/temperature". It logs the result
 * of the publish operation.
 *
 * @param[in] temperature The temperature value to be published (in °C).
 */
static void mqtt_publish_temperature(float temperature) {
    char message_buffer[256] = {0};
    char time_buffer[64]     = {0};
    char channel[64]         = {0};

    get_timestamp_in_iso_format(time_buffer, sizeof(time_buffer));
    snprintf(message_buffer, sizeof(message_buffer),
             "{\"timestamp\": \"%s\", \"value\": %.2f}",
             time_buffer, temperature);

    if (sniprintf(channel, sizeof(channel), "/titanium/%s/temperature", unique_id) < sizeof(channel)) {
        int msg_id = esp_mqtt_client_publish(mqtt_client, channel, message_buffer, 0, 1, 0);
        if (msg_id >= 0) {
            ESP_LOGD(TAG, "Message published successfully, msg_id=%d", msg_id);
        } else {
            ESP_LOGE(TAG, "Failed to publish message");
        }
    } else {
        ESP_LOGE(TAG, "Channel buffer too small");
    }
}

/**
 * @brief Publish the humidity data to the MQTT broker.
 *
 * This function formats the humidity value as a JSON string with a timestamp
 * and publishes it to the MQTT topic "/titanium/1/humidity". It logs the result
 * of the publish operation.
 *
 * @param[in] humidity The humidity value to be published (in percentage).
 */
static void mqtt_publish_humidity(float humidity) {
    char message_buffer[256] = {0};
    char time_buffer[64]     = {0};
    char channel[64]         = {0};

    get_timestamp_in_iso_format(time_buffer, sizeof(time_buffer));
    snprintf(message_buffer, sizeof(message_buffer),
             "{\"timestamp\": \"%s\", \"value\": %.2f}",
             time_buffer, humidity);
    if (sniprintf(channel, sizeof(channel), "/titanium/%s/humidity", unique_id) < sizeof(channel)) {
        int msg_id = esp_mqtt_client_publish(mqtt_client, channel, message_buffer, 0, 1, 0);
        if (msg_id >= 0) {
            ESP_LOGD(TAG, "Message published successfully, msg_id=%d", msg_id);
        } else {
            ESP_LOGE(TAG, "Failed to publish message");
        }
    } else {
        ESP_LOGE(TAG, "Channel buffer too small");
    }
}

/**
 * @brief Publishes sensor data to the MQTT topic.
 *
 * Retrieves sensor data from the queue, formats it into a JSON string,
 * and publishes it to the configured MQTT topic.
 */
static void mqtt_publish_data(void) {
    if (mqtt_client) {
        temperature_data_st temperature_data = {0};
        if (xQueueReceive(global_config->app_data_queue, &temperature_data, pdMS_TO_TICKS(100))) {
            mqtt_publish_temperature(temperature_data.temperature);
            mqtt_publish_humidity(temperature_data.humidity);
        }
    }
}

/**
 * @brief Main MQTT execution task.
 *
 * Manages the MQTT connection and periodically checks for new sensor data
 * to publish. It ensures the client is started and stopped based on Wi-Fi
 * connection status.
 *
 * @param[in] pvParameters User-defined parameters (not used).
 */
void mqtt_client_task_execute(void* pvParameters) {
    global_config = (global_config_st*)pvParameters;
    if ((mqtt_client_task_initialize() != ESP_OK) ||
        (global_config->firmware_event_group == NULL) ||
        (global_config->app_data_queue == NULL) ||
        (global_config == NULL)) {
        ESP_LOGE(TAG, "Failed to initialize MQTT task");
        vTaskDelete(NULL);
    }

    while (1) {
        EventBits_t firmware_event_bits = xEventGroupWaitBits(global_config->firmware_event_group,
                                                              WIFI_CONNECTED_STA,
                                                              pdFALSE,
                                                              pdFALSE,
                                                              pdMS_TO_TICKS(100));

        if (is_mqtt_connected) {
            if ((firmware_event_bits & WIFI_CONNECTED_STA) == 0) {
                stop_mqtt_client();
            } else if (firmware_event_bits & TIME_SYNCED) {
                mqtt_publish_data();
            }
        } else {
            if (firmware_event_bits & WIFI_CONNECTED_STA) {
                start_mqtt_client();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
