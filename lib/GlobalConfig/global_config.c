#include "global_config.h"
#include "application_external_types.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "string.h"

/*
 * @brief Initialize the Non-Volatile Storage (NVS) for the device.
 *
 * This function initializes the NVS (Non-Volatile Storage) module, which is used to store
 * configuration data and other persistent information. If the NVS initialization fails due
 * to no free pages or a new version of NVS being found, it will erase the existing NVS data
 * and reinitialize it.
 *
 * @return
 * - ESP_OK on successful initialization.
 * - Other error codes from `nvs_flash_init()` in case of failure.
 */
static esp_err_t initialize_nvs(void) {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result;
}

/**
 * @brief Initializes the global configuration structure.
 *
 * This function initializes the global configuration by setting up the firmware event
 * group and MQTT topics for "temperature" and "humidity". It ensures that memory
 * resources are available and initializes each MQTT topic with a unique queue for
 * sensor data. It also handles error cases for invalid arguments, memory allocation
 * failures, and topic initialization failures.
 *
 * @param config Pointer to the global configuration structure to be initialized.
 *
 * @return ESP_OK if the initialization is successful. Otherwise, returns one of the following error codes:
 *         - ESP_ERR_INVALID_ARG if the config pointer is NULL.
 *         - ESP_ERR_NO_MEM if memory allocation fails at any step.
 */
esp_err_t global_config_initialize(global_config_st *config) {
    esp_err_t result = ESP_OK;

    do {
        if (config == NULL) {
            result = ESP_ERR_INVALID_ARG;
            break;
        }

        result = initialize_nvs();

        if (result != ESP_OK) {
            break;
        }

        config->firmware_event_group = xEventGroupCreate();

        if (config->firmware_event_group == NULL) {
            result = ESP_ERR_NO_MEM;
            break;
        }

    } while (0);

    return result;
}