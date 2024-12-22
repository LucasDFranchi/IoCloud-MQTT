#include "mqtt_client_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "network.h"
#include "temperature_monitor.h"

/**
 * @file
 * @brief MQTT client task implementation for managing MQTT connection and publishing sensor data.
 */
static const char* TAG                      = "MQTTProcess";
static esp_mqtt_client_handle_t mqtt_client = {0};
static bool is_mqtt_connected               = false;

/**
 * @brief Event group to signal the Wi-Fi status.
 *
 * This external event group is used to monitor the Wi-Fi connection status
 * and determine when to start or stop the MQTT client.
 */
extern EventGroupHandle_t wifi_event_group;

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
            ESP_LOGI(TAG, "MQTT_EVENT_DATA: Topic=%.*s, Data=%.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGI(TAG, "Other MQTT event: %d", event->event_id);
            break;
    }
}

/**
 * @brief Initializes the MQTT client and its configuration.
 *
 * This function sets up the MQTT client, configures its parameters, and
 * registers the event handler callback.
 */
static void mqtt_client_initialize(void) {
    esp_mqtt_client_config_t mqtt_cfg = {0};
    esp_err_t result                  = ESP_OK;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    result += esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    result += esp_mqtt_client_set_uri(mqtt_client, "mqtt://mqtt.eclipseprojects.io");
    result += esp_mqtt_set_config(mqtt_client, &mqtt_cfg);
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
 * @brief Publishes sensor data to the MQTT topic.
 *
 * Retrieves sensor data from the queue, formats it into a JSON string,
 * and publishes it to the configured MQTT topic.
 */
void mqtt_publish_data(void) {
    if (mqtt_client) {
        temperature_data_st temperature_data = {0};
        if (xQueueReceive(sensor_data_queue, &temperature_data, pdMS_TO_TICKS(100))) {
            char message_buffer[128] = {0};
            snprintf(message_buffer, sizeof(message_buffer),
                     "{\"timestamp\": 0, \"temperature\": \"%.2f°C\"}",
                     temperature_data.temperature);

            int msg_id = esp_mqtt_client_publish(mqtt_client, "/titanium/1/temperature", message_buffer, 0, 1, 0);
            if (msg_id >= 0) {
                ESP_LOGI(TAG, "Message published successfully, msg_id=%d", msg_id);
            } else {
                ESP_LOGE(TAG, "Failed to publish message");
            }
            memset(message_buffer, 0, sizeof(message_buffer));
            snprintf(message_buffer, sizeof(message_buffer),
                     "{\"timestamp\": 0, \"humidity\": \"%.2f%%\"}",
                     temperature_data.humidity);

            msg_id = esp_mqtt_client_publish(mqtt_client, "/titanium/1/humidity", message_buffer, 0, 1, 0);
            if (msg_id >= 0) {
                ESP_LOGI(TAG, "Message published successfully, msg_id=%d", msg_id);
            } else {
                ESP_LOGE(TAG, "Failed to publish message");
            }
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
void mqtt_client_execute(void* pvParameters) {
    mqtt_client_initialize();

    while (1) {
        EventBits_t wifi_bits = xEventGroupWaitBits(wifi_event_group,
                                                    WIFI_CONNECTED_STA,
                                                    pdFALSE,
                                                    pdFALSE,
                                                    pdMS_TO_TICKS(100));

        if (is_mqtt_connected) {
            if ((wifi_bits & WIFI_CONNECTED_STA) == 0) {
                stop_mqtt_client();
            } else {
                mqtt_publish_data();
            }
        } else {
            if (wifi_bits & WIFI_CONNECTED_STA) {
                start_mqtt_client();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
