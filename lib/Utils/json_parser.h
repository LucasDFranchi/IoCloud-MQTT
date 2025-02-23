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
#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "esp_err.h"
#include "application_external_types.h"

/**
 * @brief Parses a JSON string to extract "block", "sector", and "data" values for a write command.
 *
 * This function tokenizes a JSON string using `jsmn` and extracts the values for "block",
 * "sector", and "data". The function ensures proper type validation and handles errors
 * such as invalid JSON format or buffer overflows.
 *
 * @param[in]  json_string   Pointer to the JSON string.
 * @param[out] command_write Pointer to the `command_write_st` structure where extracted values will be stored.
 *
 * @return
 * - ESP_OK              : If parsing is successful.
 * - ESP_ERR_INVALID_ARG : If input pointers are NULL or JSON structure is invalid.
 * - ESP_ERR_INVALID_SIZE: If the data array is too large.
 * - ESP_FAIL            : If token parsing fails.
 */
esp_err_t parse_json_temperature_config(const char *json_string, size_t size, temperature_config_st *temperature_config);

#endif  // JSON_PARSER_H
