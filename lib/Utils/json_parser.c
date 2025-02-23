#include "json_parser.h"
#include "application_external_types.h"
#include "esp_err.h"
#include "jsmn.h"
#include <string.h>

#define MAX_TOKENS 64

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
static esp_err_t json_compare_key(const char *json, jsmntok_t *tok, const char *key) {
    esp_err_t result = ESP_OK;

    do {
        if (json == NULL || tok == NULL || key == NULL) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        if (tok->type != JSMN_STRING) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        size_t key_length   = strlen(key);
        size_t token_length = (size_t)(tok->end - tok->start);
        if (key_length != token_length) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }

        if (strncmp(json + tok->start, key, token_length) != 0) {
            result = ESP_ERR_NOT_FOUND;
            break;
        }

    } while (0);

    return result;
}

/**
 * @brief Parses a JSON string to extract the "time_interval" value for temperature configuration.
 *
 * This function tokenizes a JSON string using `jsmn` and extracts the value for "time_interval".
 * It performs validation to ensure proper structure, size constraints, and token parsing success.
 *
 * @param[in]  json_string   Pointer to the JSON string.
 * @param[in]  size          Size of the JSON string in bytes.
 * @param[out] temperature_config Pointer to the 'temperature_config_st' structure where the extracted value will be stored.
 *
 * @return
 * - ESP_OK              : If parsing is successful.
 * - ESP_ERR_INVALID_ARG : If input pointers are NULL.
 * - ESP_ERR_INVALID_SIZE: If the input JSON string exceeds the buffer size.
 * - ESP_ERR_NOT_FOUND   : If the "time_interval" key is missing in the JSON.
 * - ESP_FAIL            : If JSON parsing fails.
 */
esp_err_t parse_json_temperature_config(const char *json_string, size_t size, temperature_config_st *temperature_config) {
    char buffer[256]                  = {0};
    const uint8_t TIME_INTERVAL_INDEX = 1;

    if ((json_string == NULL) || (temperature_config == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (size >= sizeof(buffer)) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(buffer, json_string, size);
    buffer[size] = '\0';

    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];

    jsmn_init(&parser);
    int found_tokens = jsmn_parse(&parser, buffer, strlen(buffer), tokens, MAX_TOKENS);

    if (found_tokens < 0) {
        return ESP_FAIL;
    }

    if (json_compare_key(buffer, &tokens[TIME_INTERVAL_INDEX], "time_interval") == 0) {
        temperature_config->time_interval = atoi(buffer + tokens[TIME_INTERVAL_INDEX + 1].start);
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

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
esp_err_t parse_json_temperature_calibration(const char *json_string, size_t size, temperature_calibration_st *temperature_calibration) {
    char buffer[256]         = {0};
    const uint8_t GAIN_INDEX = 1;
    const uint8_t OFFSET     = 3;

    if ((json_string == NULL) || (temperature_calibration == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (size >= sizeof(buffer)) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(buffer, json_string, size);
    buffer[size] = '\0';

    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];

    jsmn_init(&parser);
    int found_tokens = jsmn_parse(&parser, buffer, strlen(buffer), tokens, MAX_TOKENS);

    if (found_tokens < 0) {
        return ESP_FAIL;
    }

    if (json_compare_key(buffer, &tokens[GAIN_INDEX], "gain") == 0) {
        temperature_calibration->gain = atoi(buffer + tokens[GAIN_INDEX + 1].start);
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    if (json_compare_key(buffer, &tokens[OFFSET], "offset") == 0) {
        temperature_calibration->offset = atoi(buffer + tokens[OFFSET + 1].start);
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}
