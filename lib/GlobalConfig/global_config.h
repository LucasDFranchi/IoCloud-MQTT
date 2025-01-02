#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include "events_definition.h"
#include "tasks_definition.h"

#define MQTT_MAXIMUM_TOPIC_LENTGH 64  ///< Defines the maximum length of an MQTT topic string.
#define MQTT_MAXIMUM_TOPIC_COUNT 10   ///< Defines the maximum number of MQTT topics that can be subscribed to.
#define MQTT_MAX_DATA_STRING_SIZE 9   ///< Defines the maximum size of the data string to be sent over MQTT.

/**
 * @brief Represents an MQTT topic configuration.
 *
 * This structure encapsulates the configuration for an individual MQTT topic,
 * including the topic string, Quality of Service (QoS) level, and a queue for
 * handling sensor data. It is used for defining the properties of each MQTT
 * topic to be used in the application.
 *
 * @note The topic name is stored as a fixed-size string, and care should be taken
 *       not to exceed the maximum allowed length defined by `MQTT_MAXIMUM_TOPIC_LENTGH`.
 */
typedef struct mqtt_topic_s {
    char topic[MQTT_MAXIMUM_TOPIC_LENTGH];  ///< MQTT topic string.
    uint8_t qos;                            ///< Quality of Service (QoS) level for the topic.
    QueueHandle_t queue;                    ///< External handle for the sensor data queue.
} mqtt_topic_st;

/**
 * @brief Global configuration structure for the system.
 *
 * This structure holds the global configuration parameters for the system, including
 * the event group for managing firmware states and events, as well as an array of
 * MQTT topics used for communication. Each topic has an associated queue for storing
 * sensor data, and the system supports multiple topics defined by `MQTT_MAXIMUM_TOPIC_COUNT`.
 *
 * @note This structure is initialized at system startup to configure the system's
 *       event signaling and MQTT communication channels.
 */
typedef struct global_config_s {
    EventGroupHandle_t firmware_event_group;              ///< Event group for signaling system status and events.
    mqtt_topic_st mqtt_topics[MQTT_MAXIMUM_TOPIC_COUNT];  ///< Array of MQTT topics and QoS levels.
    uint8_t initalized_mqtt_topics_count;                 ///< Number of initialized MQTT topics.
} global_config_st;

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
esp_err_t global_config_initialize(global_config_st *config);

/**
 * @brief Initializes an MQTT topic with its associated parameters.
 *
 * This function sets up the MQTT topic, assigns a queue for storing sensor
 * data, and sets the Quality of Service (QoS) level. It ensures that the topic
 * string does not exceed the allocated buffer size and that the queue is successfully created.
 *
 * @param global_config Pointer to the global_config structure.
 * @param topic_name The name of the MQTT topic to be set.
 * @param data_struct_size The size of the data structure to be queued for the topic.
 * @param qos The Quality of Service (QoS) level for the topic.
 *
 * @return ESP_OK if the initialization is successful. Otherwise, returns one of the following error codes:
 *         - ESP_ERR_INVALID_ARG if the topic name exceeds the buffer size.
 *         - ESP_ERR_NO_MEM if the queue cannot be created.
 */
esp_err_t mqtt_topic_initialize(global_config_st *global_config, const char *topic_name, uint8_t qos);

#endif /* GLOBAL_CONFIG_H */