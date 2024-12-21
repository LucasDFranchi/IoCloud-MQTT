#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @file network.h
 * @brief Network management interface for ESP32, supporting AP and STA modes.
 */

/**
 * @brief Struct representing the current network connection status.
 */
typedef struct network_status_s {
    bool is_connect_ap;   ///< Indicates if the device is connected to the Access Point.
    bool is_connect_sta;  ///< Indicates if the device is connected to a Station.
} network_status_st;

/**
 * @brief FreeRTOS event group to signal connection events.
 *
 * This event group uses two bits:
 * - WIFI_CONNECTED_STA: Device successfully connected to a network in station mode.
 * - WIFI_CONNECTED_AP: Device successfully connected to a network in ap mode.
 */
#define WIFI_CONNECTED_STA BIT0
#define WIFI_CONNECTED_AP BIT1

/**
 * @brief Set Wi-Fi credentials for connecting to a station.
 *
 * This function stores the provided SSID and password for connecting the ESP32
 * to a Wi-Fi network in station mode.
 *
 * @param[in] ssid     Pointer to the SSID string.
 * @param[in] password Pointer to the password string.
 *
 * @return ESP_OK on success, ESP_FAIL if input validation fails or other errors occur.
 */
esp_err_t network_set_credentials(const char *ssid, const char *password);

/**
 * @brief Main execution function for network management.
 *
 * This function initializes the network, starts the Wi-Fi subsystem, and
 * continuously monitors the connection status, attempting to reconnect
 * if credentials are set but the device is disconnected.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void network_execute(void *pvParameters);

#endif  // NETWORK_H
