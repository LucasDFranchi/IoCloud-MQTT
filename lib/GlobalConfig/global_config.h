#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include "events_definition.h"
#include "mqtt_client_external_types.h"
#include "tasks_definition.h"

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

#endif /* GLOBAL_CONFIG_H */