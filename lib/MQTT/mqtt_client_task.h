#ifndef MQTT_CLIENT_TASK_H
#define MQTT_CLIENT_TASK_H

/**
 * @file mqtt_client_task.h
 * @brief MQTT Client interface for handling web requests on the ESP32.
 */
#include "global_config.h"
#include "application_external_types.h"

/**
 * @brief Main execution function for the MQTT Client.
 *
 * This function initializes and starts the MQTT Client, enabling the ESP32 to
 * handle incoming requests, from the broker. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void mqtt_client_task_execute(void* pvParameters);

/**
 * @brief Initializes an MQTT topic with its associated parameters.
 *
 * This function sets up the MQTT topic, assigns a queue for storing sensor
 * data, and sets the Quality of Service (QoS) level. It ensures that the topic
 * string does not exceed the allocated buffer size and that the queue is successfully created.
 *
 * @param global_config Pointer to the global_config structure.
 * @param topic_name The name of the MQTT topic to be set.
 * @param data_info The information of the data structure to be queued for the topic.
 *
 * @return ESP_OK if the initialization is successful. Otherwise, returns one of the following error codes:
 *         - ESP_ERR_INVALID_ARG if the topic name exceeds the buffer size.
 *         - ESP_ERR_NO_MEM if the queue cannot be created.
 */
esp_err_t mqtt_topic_initialize(global_config_st *global_config,  const char *topic_name, data_info_st *data_info);

#endif /* MQTT_CLIENT_TASK_H */