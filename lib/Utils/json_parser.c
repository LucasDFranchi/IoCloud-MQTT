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
esp_err_t parse_json_command_write(const char *json_string, size_t size, command_write_st *command_write) {
    esp_err_t result                 = ESP_OK;
    char buffer[256]                 = {0};
    const uint8_t BLOCK_TOKEN_INDEX  = 1;
    const uint8_t SECTOR_TOKEN_INDEX = 3;
    const uint8_t DATA_TOKEN_INDEX   = 5;

    do {
        if ((json_string == NULL) || (command_write == NULL)) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        if (size >= sizeof(buffer)) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }

        memcpy(buffer, json_string, size);
        buffer[size] = '\0';

        jsmn_parser parser;
        jsmntok_t tokens[MAX_TOKENS];

        jsmn_init(&parser);
        int found_tokens = jsmn_parse(&parser, buffer, strlen(buffer), tokens, MAX_TOKENS);

        if (found_tokens < 0) {
            result = ESP_FAIL;
            break;
        }

        if (json_compare_key(buffer, &tokens[BLOCK_TOKEN_INDEX], "block") == 0) {
            command_write->block = atoi(buffer + tokens[BLOCK_TOKEN_INDEX + 1].start);
        }
        if (json_compare_key(buffer, &tokens[SECTOR_TOKEN_INDEX], "sector") == 0) {
            command_write->sector = atoi(buffer + tokens[SECTOR_TOKEN_INDEX + 1].start);
        }
        if (json_compare_key(buffer, &tokens[DATA_TOKEN_INDEX], "data") == 0) {
            uint8_t array_size = tokens[DATA_TOKEN_INDEX + 1].size;
            if (array_size > sizeof(command_write->data)) {
                result = ESP_ERR_INVALID_SIZE;
                break;
            }
            if (tokens[DATA_TOKEN_INDEX + 1].type != JSMN_ARRAY) {
                result = ESP_ERR_INVALID_ARG;
                break;
            }
            for (uint8_t i = 0; i < array_size; i++) {
                command_write->data[i] = atoi(buffer + tokens[DATA_TOKEN_INDEX + 2 + i].start);
            }
        }
        if (result != ESP_OK) {
            break;
        }
    } while (0);

    return result;
}

/**
 * @brief Parses a JSON string to extract "block", "sector", and "mode" values for a config command.
 *
 * This function tokenizes a JSON string using `jsmn` and extracts the values for "block",
 * "sector", and "mode". The function ensures proper type validation.
 *
 * @param[in]  json_string    Pointer to the JSON string.
 * @param[out] command_config Pointer to the `command_config_st` structure where extracted values will be stored.
 *
 * @return
 * - ESP_OK              : If parsing is successful.
 * - ESP_ERR_INVALID_ARG : If input pointers are NULL or JSON structure is invalid.
 * - ESP_FAIL            : If token parsing fails.
 */
esp_err_t parse_json_command_config(const char *json_string, size_t size, command_config_st *command_config) {
    esp_err_t result                 = ESP_OK;
    char buffer[256]                 = {0};
    const uint8_t BLOCK_TOKEN_INDEX  = 1;
    const uint8_t SECTOR_TOKEN_INDEX = 3;
    const uint8_t MODE_TOKEN_INDEX   = 5;

    do {
        if ((json_string == NULL) || (command_config == NULL)) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        if (size >= sizeof(buffer)) {
            result = ESP_ERR_INVALID_SIZE;
            break;
        }

        memcpy(buffer, json_string, size);
        buffer[size] = '\0';

        jsmn_parser parser;
        jsmntok_t tokens[MAX_TOKENS];

        jsmn_init(&parser);
        int found_tokens = jsmn_parse(&parser, buffer, strlen(buffer), tokens, MAX_TOKENS);

        if (found_tokens < 0) {
            result = ESP_FAIL;
            break;
        }
        if (json_compare_key(buffer, &tokens[BLOCK_TOKEN_INDEX], "block") == 0) {
            command_config->block = atoi(buffer + tokens[BLOCK_TOKEN_INDEX + 1].start);
        }
        if (json_compare_key(buffer, &tokens[SECTOR_TOKEN_INDEX], "sector") == 0) {
            command_config->sector = atoi(buffer + tokens[SECTOR_TOKEN_INDEX + 1].start);
        }
        if (json_compare_key(buffer, &tokens[MODE_TOKEN_INDEX], "mode") == 0) {
            command_config->mode = atoi(buffer + tokens[MODE_TOKEN_INDEX + 1].start);
        }
        if (result != ESP_OK) {
            break;
        }
    } while (0);

    return result;
}