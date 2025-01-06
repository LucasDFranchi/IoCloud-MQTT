#ifndef MQTT_CLIENT_EXTERNAL_TYPES_H
#define MQTT_CLIENT_EXTERNAL_TYPES_H

#include "application_external_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>

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
    data_info_st data_info;                 ///< Data structure type for the topic.
    bool is_initialized;                    ///< Flag indicating whether the topic is initialized.
} mqtt_topic_st;

#endif /* MQTT_CLIENT_EXTERNAL_TYPES_H */
