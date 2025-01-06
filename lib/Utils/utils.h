/**
 * @file utils.h
 * @brief Utility functions for the ESP32 platform.
 *
 * This file contains declarations of helper functions to simplify common tasks
 * on the ESP32, such as time management and formatting.
 *
 * The utility functions provided are designed to be modular and reusable,
 * enhancing code maintainability and reducing redundancy across projects.
 */
#ifndef UTILS_H
#define UTILS_H

#include "esp_err.h"

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
esp_err_t get_timestamp_in_iso_format(char* buffer, size_t buffer_size);

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
void get_unique_id(char *unique_id, size_t max_len);

/**
 * @brief Formats an array of characters into a JSON-like string representation.
 *
 * This function takes an array of characters (`arr`) and writes it into the `buffer`
 * as a JSON-like string representation, e.g., `[a,b,c]`. The function ensures that 
 * no buffer overflow occurs. If there is not enough space in the buffer to write 
 * the entire output, the function returns 0 to indicate an error.
 *
 * @param buffer   Pointer to the buffer where the formatted string will be written.
 *                 The caller must ensure the buffer is large enough to hold the output.
 * @param arr      Pointer to the array of characters to format.
 * @param arr_size The number of elements in the array.
 * @param max_size The maximum size of the buffer, including space for the null terminator.
 *
 * @return The total number of characters written to the buffer, excluding the null terminator,
 *         or 0 if there is insufficient space in the buffer to write the entire output.
 *
 * @note If the function returns 0, the buffer content is considered invalid and should not
 *       be used.
 *
 * @example
 * char buffer[20];
 * char arr[] = {'a', 'b', 'c'};
 * size_t written = snprintf_array(buffer, arr, 3, sizeof(buffer));
 * if (written > 0) {
 *     printf("Formatted array: %s\n", buffer); // Output: [a,b,c]
 * } else {
 *     printf("Error: Not enough space in the buffer.\n");
 * }
 */
size_t snprintf_array(char* buffer, uint8_t* arr, uint32_t arr_size, uint32_t max_size);

#endif  // UTILS_H
