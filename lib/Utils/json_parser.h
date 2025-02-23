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
 * @brief Compares a JSON key with a given string.
 *
 * This function checks whether the provided JSON token represents a string
 * that matches the given key. It ensures type correctness, size matching,
 * and performs a direct string comparison.
 *
 * @param[in]  json  Pointer to the JSON string buffer.
 * @param[in]  tok   Pointer to the JSON token to compare.
 * @param[in]  key   Pointer to the key string to match.
 *
 * @return
 * - ESP_OK              : If the token matches the key.
 * - ESP_ERR_INVALID_ARG : If any input pointer is NULL or the token is not a string.
 * - ESP_ERR_INVALID_SIZE: If the key length and token length do not match.
 * - ESP_ERR_NOT_FOUND   : If the key does not match the token.
 */
esp_err_t parse_json_temperature_config(const char *json_string, size_t size, temperature_config_st *temperature_config);

/**
 * @brief Parses a JSON string to extract "gain" and "offset" values for temperature calibration.
 *
 * This function tokenizes a JSON string using `jsmn` and extracts the values for "gain" and "offset".  
 * It validates the JSON structure, ensures size constraints, and checks for token parsing success.  
 *
 * @param[in]  json_string   Pointer to the JSON string.
 * @param[in]  size          Size of the JSON string in bytes.
 * @param[out] temperature_calibration Pointer to the 'temperature_calibration_st' structure where the extracted values will be stored.
 *
 * @return
 * - ESP_OK              : If parsing is successful.
 * - ESP_ERR_INVALID_ARG : If input pointers are NULL.
 * - ESP_ERR_INVALID_SIZE: If the input JSON string exceeds the buffer size.
 * - ESP_ERR_NOT_FOUND   : If either the "gain" or "offset" key is missing in the JSON.
 * - ESP_FAIL            : If JSON parsing fails.
 */
esp_err_t parse_json_temperature_calibration(const char *json_string, size_t size, temperature_calibration_st *temperature_calibration);

#endif  // JSON_PARSER_H
