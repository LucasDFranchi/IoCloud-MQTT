#include "global_config.h"
#include "application_external_types.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "string.h"

uint8_t const MAX_QUEUE_SIZE = 100;

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
static esp_err_t initialize_nvs(void) {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result;
}

/**
 * @brief Initializes an MQTT topic with its associated parameters.
 *
 * This function sets up the MQTT topic, assigns a queue for storing sensor
 * data, and sets the Quality of Service (QoS) level. It ensures that the topic
 * string does not exceed the allocated buffer size and that the queue is successfully created.
 *
 * @param topic Pointer to the mqtt_topic_st structure to be initialized.
 * @param topic_name The name of the MQTT topic to be set.
 * @param data_struct_size The size of the data structure to be queued for the topic.
 * @param qos The Quality of Service (QoS) level for the topic.
 *
 * @return ESP_OK if the initialization is successful. Otherwise, returns one of the following error codes:
 *         - ESP_ERR_INVALID_ARG if the topic name exceeds the buffer size.
 *         - ESP_ERR_NO_MEM if the queue cannot be created.
 */
esp_err_t mqtt_topic_initialize(global_config_st *global_config, const char *topic_name, uint8_t qos) {
    if (global_config == NULL || topic_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (global_config->initalized_mqtt_topics_count >= MQTT_MAXIMUM_TOPIC_COUNT) {
        return ESP_ERR_NO_MEM;
    }

    mqtt_topic_st *topic = &global_config->mqtt_topics[global_config->initalized_mqtt_topics_count++];
    
    size_t topic_length = snprintf(topic->topic, sizeof(topic->topic), "%s", topic_name);
    if (topic_length >= sizeof(topic->topic)) {
        return ESP_ERR_INVALID_ARG;
    }

    topic->queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(generic_sensor_data_st));
    if (topic->queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    topic->qos = qos;

    return ESP_OK;
}

/**
 * @brief Initializes the global configuration structure.
 *
 * This function initializes the global configuration by setting up the firmware event
 * group and MQTT topics for "temperature" and "humidity". It ensures that memory
 * resources are available and initializes each MQTT topic with a unique queue for
 * sensor data. It also handles error cases for invalid arguments, memory allocation
 * failures, and topic initialization failures.
 *
 * @param config Pointer to the global configuration structure to be initialized.
 *
 * @return ESP_OK if the initialization is successful. Otherwise, returns one of the following error codes:
 *         - ESP_ERR_INVALID_ARG if the config pointer is NULL.
 *         - ESP_ERR_NO_MEM if memory allocation fails at any step.
 */
esp_err_t global_config_initialize(global_config_st *config) {
    esp_err_t result = ESP_OK;

    do {
        if (config == NULL) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        result = initialize_nvs();

        if (result != ESP_OK) {
            break;
        }

        config->firmware_event_group = xEventGroupCreate();

        if (config->firmware_event_group == NULL) {
            result = ESP_ERR_NO_MEM;
            break;
        }

    } while (0);

    return result;
}