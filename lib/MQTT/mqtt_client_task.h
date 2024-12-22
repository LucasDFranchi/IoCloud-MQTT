#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

/**
 * @file mqtt_client.h
 * @brief MQTT Client interface for handling web requests on the ESP32.
 */

/**
 * @brief Main execution function for the MQTT Client.
 *
 * This function initializes and starts the MQTT Client, enabling the ESP32 to
 * handle incoming requests, from the broker. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void mqtt_client_execute(void* pvParameters);

#endif /* MQTT_CLIENT_H */
