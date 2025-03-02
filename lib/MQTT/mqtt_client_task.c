#include "mqtt_client_task.h"
#include "freertos/FreeRTOS.h"
#include "json_parser.h"
#include "logger.h"
#include "mqtt_client.h"
#include "network_task.h"
#include "utils.h"
#include <inttypes.h>

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
static const char* TAG                      = "MQTT Task";  ///< Log tag for MQTT task.
static global_config_st* global_config      = NULL;         ///< Pointer to the global configuration structure.
static bool is_mqtt_connected               = false;        ///< MQTT connection status.
static esp_mqtt_client_handle_t mqtt_client = {0};          ///< MQTT client handle.
static char unique_id[13]                   = {0};          ///< Device unique ID (12 chars + null terminator).
static const uint8_t MAX_QUEUE_SIZE         = 100;          ///< Maximum queue size for storing sensor data.

static void mqtt_subscribe(void);
static void mqtt_subscribe_topic_callback(const char* topic, const char* event_data, size_t event_data_len);

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
            logger_print(INFO, TAG, "MQTT_EVENT_CONNECTED");
            is_mqtt_connected = true;
            mqtt_subscribe();
            break;

        case MQTT_EVENT_DISCONNECTED:
            logger_print(INFO, TAG, "MQTT_EVENT_DISCONNECTED");
            is_mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA:
            logger_print(DEBUG, TAG, "MQTT_EVENT_DATA: Topic=%.*s, Data=%.*s",
                         event->topic_len, event->topic,
                         event->data_len, event->data);
            mqtt_subscribe_topic_callback(event->topic, event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            logger_print(ERR, TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            logger_print(DEBUG, TAG, "MQTT event: %d", event->event_id);
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
        logger_print(INFO, TAG, "MQTT client started");
    } else {
        logger_print(ERR, TAG, "MQTT client not initialized");
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
        logger_print(INFO, TAG, "MQTT client stopped");
    }
}

/**
 * @brief Publishes an MQTT response message for a read operation.
 *
 * This function retrieves a read response from the queue, formats it into a JSON message,
 * and writes it into the provided buffer.
 *
 * @param[in] mqtt_topic Pointer to the MQTT topic structure.
 * @param[in] timestamp Timestamp string for the message.
 * @param[in] message_buffer_out_len Maximum size of the message buffer.
 * @param[out] message_buffer Buffer to store the formatted message.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_ARG if any argument is NULL.
 *      - ESP_ERR_NOT_FOUND if no message is available in the queue.
 *      - ESP_ERR_INVALID_SIZE if formatting errors occur.
 *      - ESP_ERR_NO_MEM if the message buffer is too small.
 */
static esp_err_t mqtt_publish_response_read(const mqtt_topic_st* mqtt_topic,
                                            const char* timestamp,
                                            size_t message_buffer_out_len,
                                            char* message_buffer) {
    char array_string[256]         = {0};
    char uuid_string[16]           = {0};
    response_read_st response_read = {0};
    esp_err_t result               = ESP_FAIL;

    do {
        if (timestamp == NULL) {
            logger_print(ERR, TAG, "Timestamp is NULL");
            result = ESP_ERR_INVALID_ARG;
            break;
        }
        if (message_buffer == NULL) {
            logger_print(ERR, TAG, "Message buffer is NULL");
            result = ESP_ERR_INVALID_ARG;
            break;
        }
        if (mqtt_topic == NULL) {
            logger_print(ERR, TAG, "MQTT topic is NULL");
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        if (!xQueueReceive(global_config->mqtt_topics[DATA_STRUCT_RESPONSE_READ].queue, &response_read, pdMS_TO_TICKS(100))) {
            result = ESP_ERR_NOT_FOUND;
            break;
        }

        size_t uuid_size = snprintf(uuid_string,
                                    sizeof(uuid_string),
                                    "%" PRIu64,
                                    response_read.uuid.integer);
        if (uuid_size >= sizeof(uuid_string)) {
            result = ESP_ERR_INVALID_SIZE;
            logger_print(WARN, TAG, "Failed to format uuid");
            break;
        }

        size_t array_size = snprintf_array(array_string,
                                           response_read.data,
                                           sizeof(response_read.data),
                                           sizeof(array_string));
        if (array_size >= sizeof(array_string)) {
            result = ESP_ERR_INVALID_SIZE;
            logger_print(WARN, TAG, "Failed to format array");
            break;
        }

        size_t message_size = snprintf(message_buffer,
                                       message_buffer_out_len,
                                       "{\"timestamp\": \"%s\", \"uid\": %s, \"block\": %d, \"sector\": %d, \"data\": %s}",
                                       timestamp,
                                       uuid_string,
                                       response_read.block,
                                       response_read.sector,
                                       array_string);

        if ((message_size >= message_buffer_out_len) || (message_size == 0)) {
            result = ESP_ERR_NO_MEM;
            logger_print(WARN, TAG, "mqtt_publish_response_read - Failed to format message");
            break;
        }

        result = ESP_OK;

    } while (0);

    return result;
}


/**
 * @brief Publishes an MQTT response message for a write operation.
 *
 * This function retrieves a write response from the queue, formats it into a JSON message,
 * and writes it into the provided buffer.
 *
 * @param[in] mqtt_topic Pointer to the MQTT topic structure.
 * @param[in] timestamp Timestamp string for the message.
 * @param[in] message_buffer_out_len Maximum size of the message buffer.
 * @param[out] message_buffer Buffer to store the formatted message.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_ARG if any argument is NULL.
 *      - ESP_ERR_NOT_FOUND if no message is available in the queue.
 *      - ESP_ERR_INVALID_SIZE if formatting errors occur.
 *      - ESP_ERR_NO_MEM if the message buffer is too small.
 */
static esp_err_t mqtt_publish_response_write(const mqtt_topic_st* mqtt_topic,
                                             const char* timestamp,
                                             size_t message_buffer_out_len,
                                             char* message_buffer) {
    response_write_st response_write = {0};
    char uuid_string[16]             = {0};
    esp_err_t result                 = ESP_FAIL;

    do {
        if (timestamp == NULL) {
            logger_print(ERR, TAG, "Timestamp is NULL");
            result = ESP_ERR_INVALID_ARG;
            break;
        }
        if (message_buffer == NULL) {
            logger_print(ERR, TAG, "Message buffer is NULL");
            result = ESP_ERR_INVALID_ARG;
            break;
        }
        if (mqtt_topic == NULL) {
            logger_print(ERR, TAG, "MQTT topic is NULL");
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        if (!xQueueReceive(global_config->mqtt_topics[DATA_STRUCT_RESPONSE_WRITE].queue, &response_write, pdMS_TO_TICKS(100))) {
            result = ESP_ERR_NOT_FOUND;
            break;
        }

        size_t uuid_size = snprintf(uuid_string,
                                    sizeof(uuid_string),
                                    "%" PRIu64,
                                    response_write.uuid.integer);
        if (uuid_size >= sizeof(uuid_string)) {
            result = ESP_ERR_INVALID_SIZE;
            logger_print(WARN, TAG, "Failed to format uuid");
            break;
        }

        size_t message_size = snprintf(message_buffer,
                                       message_buffer_out_len,
                                       "{\"timestamp\": \"%s\", \"uid\": %s, \"block\": %d, \"sector\": %d, \"status\": %d}",
                                       timestamp,
                                       uuid_string,
                                       response_write.block,
                                       response_write.sector,
                                       response_write.status);

        if ((message_size >= message_buffer_out_len) || (message_size == 0)) {
            result = ESP_ERR_NO_MEM;
            logger_print(WARN, TAG, "mqtt_publish_response_write - Failed to format message");
            break;
        }

        result = ESP_OK;

    } while (0);

    return result;
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
static void mqtt_publish_topic(const mqtt_topic_st* mqtt_topic) {
    char message_buffer[512] = {0};
    char time_buffer[64]     = {0};
    char channel[64]         = {0};
    esp_err_t result         = ESP_FAIL;

    do {
        get_timestamp_in_iso_format(time_buffer, sizeof(time_buffer));

        switch (mqtt_topic->data_info.type) {
            case DATA_STRUCT_RESPONSE_READ:
                result = mqtt_publish_response_read(mqtt_topic, time_buffer, sizeof(message_buffer), message_buffer);
                break;
            case DATA_STRUCT_RESPONSE_WRITE:
                result = mqtt_publish_response_write(mqtt_topic, time_buffer, sizeof(message_buffer), message_buffer);
                break;
            default:
                logger_print(ERR, TAG, "Invalid data type");
                break;
        }
        if (result == ESP_ERR_NOT_FOUND) {
            break;
        } else if (result != ESP_OK) {
            logger_print(ERR, TAG, "mqtt_publish_topic - Failed to format message");
            break;
        }

        size_t channel_size = snprintf(channel, sizeof(channel), "/titanium/%s/%s", unique_id, mqtt_topic->topic);

        if (channel_size >= sizeof(channel)) {
            logger_print(WARN, TAG, "Channel buffer too small");
            break;
        }

        int msg_id = esp_mqtt_client_publish(mqtt_client, channel, message_buffer, 0, 1, 0);
        if (msg_id >= 0) {
            logger_print(DEBUG, TAG, "Message published successfully, msg_id=%d", msg_id);
        } else {
            logger_print(ERR, TAG, "Failed to publish message");
        }
    } while (0);
}

/**
 * @brief Callback function for handling subscribed MQTT topics.
 *
 * This function processes incoming MQTT messages by matching the topic with
 * configured topics, parsing the payload based on the expected data structure,
 * and placing the parsed data into the appropriate queue.
 *
 * @param topic The MQTT topic associated with the received message.
 * @param event_data The raw JSON payload of the received message.
 * @param event_data_len The length of the event_data string.
 */
static void mqtt_subscribe_topic_callback(const char* topic, const char* event_data, size_t event_data_len) {
    if ((topic == NULL) || (event_data == NULL) || (event_data_len == 0)) {
        logger_print(ERR, TAG, "Invalid arguments");
        return;
    }

    for (int i = 0; i < global_config->initalized_mqtt_topics_count; i++) {
        if (global_config->mqtt_topics[i].queue == NULL) {
            logger_print(ERR, TAG, "Queue for topic %s is NULL", global_config->mqtt_topics[i].topic);
            continue;
        }
        if (global_config->mqtt_topics[i].data_info.direction == PUBLISH) {
            continue;
        }

        if (strstr(topic, global_config->mqtt_topics[i].topic) == NULL) {
            continue;
        }

        BaseType_t result      = pdFAIL;
        esp_err_t parse_result = ESP_FAIL;

        switch (global_config->mqtt_topics[i].data_info.type) {
            case DATA_STRUCT_COMMAND_WRITE:
                command_write_st command_write = {0};

                parse_result = parse_json_command_write(event_data, event_data_len, &command_write);
                if (parse_result != ESP_OK) {
                    logger_print(WARN, TAG,
                                 "Failed to parse %s data - Error %d",
                                 global_config->mqtt_topics[DATA_STRUCT_COMMAND_WRITE].topic,
                                 parse_result);
                    break;
                }

                result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_COMMAND_WRITE].queue,
                                    &command_write,
                                    pdMS_TO_TICKS(100));
                if (result != pdPASS) {
                    logger_print(WARN, TAG,
                                 "Failed to send %s data to queue",
                                 global_config->mqtt_topics[DATA_STRUCT_COMMAND_WRITE].topic);
                }
                break;
            case DATA_STRUCT_COMMAND_CONFIG:
                command_config_st command_config = {0};

                parse_result = parse_json_command_config(event_data, event_data_len, &command_config);
                if (parse_result != ESP_OK) {
                    logger_print(WARN, TAG,
                                 "Failed to parse %s data - Error %d",
                                 global_config->mqtt_topics[DATA_STRUCT_COMMAND_CONFIG].topic,
                                 parse_result);
                    break;
                }

                result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_COMMAND_CONFIG].queue,
                                    &command_config,
                                    pdMS_TO_TICKS(100));
                if (result != pdPASS) {
                    logger_print(WARN, TAG,
                                 "Failed to send %s data to queue",
                                 global_config->mqtt_topics[DATA_STRUCT_COMMAND_CONFIG].topic);
                }
                break;

            default:
                logger_print(ERR, TAG, "Invalid data type");
                break;
        }
    }
}

/**
 * @brief Publishes sensor data to the MQTT topic.
 *
 * Retrieves sensor data from the queue, formats it into a JSON string,
 * and publishes it to the configured MQTT topic.
 */
static void mqtt_publish(void) {
    if (mqtt_client) {
        for (int i = 0; i < global_config->initalized_mqtt_topics_count; i++) {
            if (global_config->mqtt_topics[i].queue == NULL) {
                logger_print(ERR, TAG, "Queue for topic %s is NULL", global_config->mqtt_topics[i].topic);
                continue;
            }
            if (global_config->mqtt_topics[i].data_info.direction == PUBLISH) {
                mqtt_publish_topic(&global_config->mqtt_topics[i]);
            }
        }
    }
}

/**
 * @brief Callback function for handling subscribed MQTT topics.
 *
 * This function processes incoming MQTT messages by matching the topic with
 * configured topics, parsing the payload based on the expected data structure,
 * and placing the parsed data into the appropriate queue.
 *
 * @param topic The MQTT topic associated with the received message.
 * @param event_data The raw JSON payload of the received message.
 * @param event_data_len The length of the event_data string.
 */
static esp_err_t mqtt_subscribe_topic(mqtt_topic_st* mqtt_topic) {
    char channel[64]    = {0};
    esp_err_t result    = ESP_FAIL;
    size_t channel_size = snprintf(channel, sizeof(channel), "/titanium/%s/%s", unique_id, mqtt_topic->topic);

    if (channel_size >= sizeof(channel)) {
        logger_print(ERR, TAG, "Channel buffer too small");
        result = ESP_ERR_NO_MEM;
        return result;
    }

    int msg_id = esp_mqtt_client_subscribe(mqtt_client, channel, 1);
    if (msg_id >= 0) {
        logger_print(DEBUG, TAG, "Subscribed to topic %s, msg_id=%d", channel, msg_id);
        result = ESP_OK;
    } else {
        logger_print(ERR, TAG, "Failed to subscribe to topic %s", channel);
    }

    return result;
}

/**
 * @brief Publishes sensor data to the MQTT topic.
 *
 * Retrieves sensor data from the queue, formats it into a JSON string,
 * and publishes it to the configured MQTT topic.
 */
static void mqtt_subscribe(void) {
    if (mqtt_client) {
        for (int i = 0; i < global_config->initalized_mqtt_topics_count; i++) {
            if (global_config->mqtt_topics[i].queue == NULL) {
                logger_print(ERR, TAG, "Queue for topic %s is NULL", global_config->mqtt_topics[i].topic);
                continue;
            }
            if (global_config->mqtt_topics[i].data_info.direction == SUBSCRIBE) {
                mqtt_subscribe_topic(&global_config->mqtt_topics[i]);
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
void mqtt_client_task_execute(void* pvParameters) {
    global_config = (global_config_st*)pvParameters;
    if ((mqtt_client_task_initialize() != ESP_OK) ||
        (global_config->firmware_event_group == NULL) ||
        (global_config == NULL)) {
        logger_print(ERR, TAG, "Failed to initialize MQTT task");
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
                mqtt_publish();
            }
        } else {
            if (firmware_event_bits & WIFI_CONNECTED_STA) {
                start_mqtt_client();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Initializes an MQTT topic with its associated parameters.
 *
 * This function sets up the MQTT topic, assigns a queue for storing sensor
 * data, and sets the Quality of Service (QoS) level. It ensures that the topic
 * string does not exceed the allocated buffer size and that the queue is successfully created.
 *
 * @param global_config Pointer to the global_config structure.
 * @param topic_name The name of the MQTT topic to be set.
 * @param qos The Quality of Service (QoS) level for the topic.
 * @param data_type The type of the data structure to be queued for the topic.
 *
 * @return ESP_OK if the initialization is successful. Otherwise, returns one of the following error codes:
 *         - ESP_ERR_INVALID_ARG if the topic name exceeds the buffer size.
 *         - ESP_ERR_NO_MEM if the queue cannot be created.
 */
esp_err_t mqtt_topic_initialize(global_config_st* global_config, const char* topic_name, data_info_st* data_type) {
    if ((global_config == NULL) || (topic_name == NULL) || (data_type == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (global_config->initalized_mqtt_topics_count >= MQTT_MAXIMUM_TOPIC_COUNT) {
        return ESP_ERR_NO_MEM;
    }

    mqtt_topic_st* topic = &global_config->mqtt_topics[global_config->initalized_mqtt_topics_count++];

    topic->data_info.direction = data_type->direction;
    topic->data_info.size      = data_type->size;
    topic->data_info.type      = data_type->type;

    size_t topic_length = snprintf(topic->topic, sizeof(topic->topic), "%s", topic_name);
    if (topic_length >= sizeof(topic->topic)) {
        return ESP_ERR_INVALID_ARG;
    }

    topic->queue = xQueueCreate(MAX_QUEUE_SIZE, data_type->size);
    if (topic->queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    topic->qos = 1;

    topic->is_initialized = true;

    return ESP_OK;
}
