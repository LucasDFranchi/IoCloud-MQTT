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
