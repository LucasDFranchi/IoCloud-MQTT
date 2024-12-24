#include "utils.h"

#include <time.h>
#include <string.h>
#include "esp_err.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_log.h"

/**
 * @brief Retrieve a unique identifier for the ESP32 based on its MAC address.
 *
 * This function generates a unique ID string by retrieving the default MAC address
 * of the ESP32 and formatting it as a hexadecimal string (e.g., "24A160FFEE01").
 * If the MAC address retrieval fails, the function sets the unique ID to "UNKNOWN".
 *
 * @param[out] unique_id Buffer to store the generated unique ID string.
 * @param[in]  max_len   Maximum length of the buffer. It should be at least 13 bytes
 *                       to hold the formatted MAC address (12 characters + null terminator).
 *
 * @note The MAC address is guaranteed to be unique across devices, providing a reliable
 *       way to identify individual devices in a network.
 */
void get_unique_id(char *unique_id, size_t max_len) {
    if (unique_id == NULL || max_len < 13) { // Minimum length is 12 chars + '\0'
        ESP_LOGE("get_unique_id", "Invalid buffer or buffer size.");
        return;
    }

    uint8_t mac[6];
    esp_err_t err = esp_efuse_mac_get_default(mac);  // Retrieve default MAC address
    if (err == ESP_OK) {
        snprintf(unique_id, max_len, "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE("get_unique_id", "Failed to retrieve MAC address, error code: %d", err);
        snprintf(unique_id, max_len, "UNKNOWN");
    }

    ESP_LOGI("Utils", "Unique ID: %s", unique_id);
}

/**
 * @brief Get the current timestamp in ISO 8601 format.
 *
 * This function retrieves the current system time and formats it
 * as an ISO 8601 string (e.g., "2024-12-24T15:30:45").
 *
 * @param[out] buffer      Pointer to the buffer where the formatted timestamp will be stored.
 * @param[in]  buffer_size Size of the buffer.
 *
 * @return ESP_OK on success, or an appropriate error code on failure:
 *         - ESP_ERR_INVALID_ARG if the buffer is NULL or the size is zero.
 *         - ESP_ERR_INVALID_STATE if the system time is not set.
 *         - ESP_FAIL if time formatting fails.
 */
esp_err_t get_timestamp_in_iso_format(char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    time_t now = time(NULL);
    if (now == (time_t)(-1)) {
        return ESP_ERR_INVALID_STATE;
    }

    struct tm timeinfo;
    if (localtime_r(&now, &timeinfo) == NULL) {
        return ESP_FAIL;
    }

    if (strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S", &timeinfo) == 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}