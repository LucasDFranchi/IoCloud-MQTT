#include "application_task.h"
#include "Driver/pn532.h"
#include "application_external_types.h"
#include "esp_log.h"
#include "global_config.h"

#include "esp_err.h"

// Move to a TAG HEADER FILE
#define TAG_UUID_MAX_SIZE (7)
#define MIFARE_BLOCKS_PER_SECTOR (4)
#define MIFARE_DATA_BLOCKS_PER_SECTOR (3)
#define MIFARE_TRAILER_BLOCK (3)
#define MIFARE_BLOCK_SIZE (16)
#define MIFARE_SECTOR_SIZE (MIFARE_BLOCK_SIZE * MIFARE_BLOCKS_PER_SECTOR)

enum {
    USE_KEY_A = 0,
    USE_KEY_B,
};

typedef enum reader_mode_e {
    READ = 0,
    WRITE,
} reader_mode_et;

/**
 * @file Application.c
 * @brief Temperature monitoring and logging for temperature and humidity data.
 *
 * This module interfaces with the AHT10 temperature and humidity sensor,
 * reads the data, and logs the temperature and humidity values periodically.
 * It provides functions to initialize the sensor, execute continuous readings,
 * and log the results.
 */

enum {
    TAG_UUID = 0,  ///< Index for the temperature sensor.
    TAG_SECTORS,
    ACTIVE_SENSORS,  ///< Number of active sensors in the system.
};

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st *global_config = NULL;  ///< Global configuration structure.

static const char *TAG         = "Application Task";  ///< Tag used for logging.
static uint8_t DEFAULT_KEY_A[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// static uint8_t DEFAULT_KEY_B[]    = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
/**
 * @brief Writes data to a specific block within a sector of a MIFARE card.
 *
 * This function authenticates the trailer block of the specified sector using Key A,
 * then writes the provided data to the specified block within that sector.
 *
 * @param[in]  uid        Pointer to the UID buffer of the card.
 * @param[in]  uid_len    Length of the UID buffer.
 * @param[in]  sector     Sector number to write to (0-based index).
 * @param[in]  block      Block number within the sector to write to (0-based index, excluding trailer block).
 * @param[in]  data       Pointer to the data buffer to write. Each block requires 16 bytes of data.
 *
 * @return
 *         - ESP_OK on successful write.
 *         - ESP_FAIL if authentication fails, input is invalid, or the write operation fails.
 */
esp_err_t write_sector(uint8_t *uid, uint8_t uid_len, uint8_t sector, uint8_t block, uint8_t *data) {
    esp_err_t result     = ESP_FAIL;
    uint8_t block_offset = sector * 4;
    uint8_t block_index  = block_offset + block;

    do {
        if (block >= MIFARE_DATA_BLOCKS_PER_SECTOR) {
            ESP_LOGE(TAG, "Block %d is out of range for Sector %d.\n", block, sector);
            break;
        }

        if (!uid || !data) {
            ESP_LOGE(TAG, "Invalid input: UID or data buffer is NULL.");
            return ESP_FAIL;
        }

        if (!mifareclassic_AuthenticateBlock(uid,
                                             uid_len,
                                             block_offset + MIFARE_TRAILER_BLOCK,
                                             USE_KEY_A,
                                             DEFAULT_KEY_A)) {
            ESP_LOGE(TAG, "Authentication failed for Sector %d.\n", sector);
            break;
        }

        if (!mifareclassic_WriteDataBlock(block_index, data + (block * MIFARE_BLOCK_SIZE))) {
            ESP_LOGE(TAG, "Failed to write Block %d of Sector %d.\n", block, sector);
            break;
        }

        result = ESP_OK;
    } while (0);

    return result;
}

/**
 * @brief Writes data to all blocks within a specified sector of a MIFARE card.
 *
 * This function iterates through all blocks in the specified sector (excluding the trailer block)
 * and writes the provided data to each block.
 *
 * @param[in]  uid        Pointer to the UID buffer of the card.
 * @param[in]  uid_len    Length of the UID buffer.
 * @param[in]  sector     Sector number to write to (0-based index).
 * @param[in]  data       Pointer to the data buffer to write. The buffer must contain
 *                        at least `MIFARE_DATA_BLOCKS_PER_SECTOR * MIFARE_BLOCK_SIZE` bytes.
 *
 * @return
 *         - ESP_OK if all blocks are written successfully.
 *         - ESP_FAIL if any block write operation fails or input is invalid.
 */
esp_err_t write_all_block_sectors(uint8_t *uid, uint8_t uid_len, uint8_t sector, uint8_t *data) {
    esp_err_t result = ESP_OK;

    do {
        if (!uid || !data) {
            ESP_LOGE(TAG, "Invalid input: UID or data buffer is NULL.");
            return ESP_FAIL;
        }

        for (int i = 0; i < MIFARE_DATA_BLOCKS_PER_SECTOR; i++) {
            if (write_sector(uid, uid_len, sector, i, data) != ESP_OK) {
                result = ESP_FAIL;
                break;
            }
        }

    } while (0);

    return result;
}

/**
 * @brief Reads a data block from a specified sector of a MIFARE Classic card.
 *
 * This function authenticates the specified sector using the default key A
 * and then reads the data from the specified block. The data is written into
 * the provided buffer if the operation is successful.
 *
 * @param uid Pointer to the UID of the card.
 * @param uid_len Length of the UID in bytes.
 * @param sector The sector number to be accessed.
 * @param block The block number within the sector to read.
 * @param read_data_len Size of the output buffer in bytes. Must be at least
 *                      `MIFARE_BLOCK_SIZE` (16 bytes).
 * @param read_data Pointer to the buffer where the read data will be stored.
 *
 * @return
 *  - ESP_OK on success.
 *  - ESP_ERR_INVALID_ARG if the inputs are invalid (e.g., NULL pointers, block out of range).
 *  - ESP_ERR_INVALID_SIZE if the output buffer size is insufficient.
 *  - ESP_FAIL on fail.
 */
esp_err_t read_sector(uint8_t *uid, uint8_t uid_len, uint8_t sector, uint8_t block, uint8_t read_data_len, uint8_t *read_data) {
    esp_err_t result     = ESP_FAIL;
    uint8_t block_offset = sector * 4;
    uint8_t block_index  = block_offset + block;

    do {
        if (!uid || !read_data) {
            result = ESP_ERR_INVALID_ARG;
            ESP_LOGE(TAG, "Invalid input: UID or data buffer is NULL.");
            break;
        }

        if (block >= MIFARE_DATA_BLOCKS_PER_SECTOR) {
            result = ESP_ERR_INVALID_ARG;
            ESP_LOGE(TAG, "Block %d is out of range for Sector %d.", block, sector);
            break;
        }

        if (read_data_len < MIFARE_BLOCK_SIZE) {
            result = ESP_ERR_INVALID_SIZE;
            ESP_LOGE(TAG, "Output buffer size is too small for the data.");
            break;
        }

        if (!mifareclassic_AuthenticateBlock(uid,
                                             uid_len,
                                             block_offset + MIFARE_TRAILER_BLOCK,
                                             USE_KEY_A,
                                             DEFAULT_KEY_A)) {
            ESP_LOGE(TAG, "Authentication failed for Sector %d.", sector);
            break;
        }

        if (!mifareclassic_ReadDataBlock(block_index, read_data + (block * MIFARE_BLOCK_SIZE))) {
            ESP_LOGE(TAG, "Failed to read Block %d of Sector %d.", block, sector);
            break;
        }

        result = ESP_OK;
    } while (0);

    return result;
}

/**
 * @brief Reads all data blocks from a specified sector of a MIFARE Classic card.
 *
 * This function reads all blocks within a given sector and stores the data into the provided buffer.
 * If any read operation fails, it returns an error, and further reading stops.
 *
 * @param uid Pointer to the UID of the card.
 * @param uid_len Length of the UID in bytes.
 * @param sector The sector number to be accessed.
 * @param read_data_len Size of the output buffer in bytes. Must be at least `MIFARE_BLOCK_SIZE * MIFARE_DATA_BLOCKS_PER_SECTOR`.
 * @param read_data Pointer to the buffer where the read data will be stored.
 *
 * @return
 *  - ESP_OK on success.
 *  - ESP_FAIL on fail.
 */
esp_err_t read_all_block_sectors(uint8_t *uid, uint8_t uid_len, uint8_t sector, uint8_t read_data_len, uint8_t *read_data) {
    esp_err_t result = ESP_OK;

    do {
        if (!uid || !read_data) {
            ESP_LOGE(TAG, "Invalid input: UID or data buffer is NULL.");
            return ESP_FAIL;
        }

        if (read_data_len < MIFARE_BLOCK_SIZE) {
            result = ESP_ERR_INVALID_SIZE;
            ESP_LOGE(TAG, "Output buffer size is too small for the data.");
            break;
        }

        for (int i = 0; i < MIFARE_DATA_BLOCKS_PER_SECTOR; i++) {
            if (read_sector(uid, uid_len, sector, i, read_data_len, read_data) != ESP_OK) {
                result = ESP_FAIL;
                break;
            }
        }

    } while (0);

    return result;
}

/**
 * @brief Initializes the temperature monitor.
 *
 * This function calls the `aht10_init` function to initialize the AHT10
 * temperature and humidity sensor. If initialization fails, the task
 * will be deleted.
 *
 * @return ESP_OK on successful initialization, ESP_FAIL on failure.
 */
static esp_err_t application_task_initialize(void) {
    esp_err_t result = ESP_OK;

    if (!init_PN532_I2C(GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_19, GPIO_NUM_18, I2C_NUM_0)) {
        ESP_LOGE(TAG, "Failed to initialize PN532");
        return ESP_FAIL;
    }

    uint32_t versiondata = getPN532FirmwareVersion();
    if (!versiondata) {
        ESP_LOGI(TAG, "Didn't find PN53x board - %ld", versiondata);
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    ESP_LOGI(TAG, "Found chip PN5 %ld", (versiondata >> 24) & 0xFF);
    ESP_LOGI(TAG, "Firmware ver. %ld.%ld", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);

    result = SAMConfig() ? ESP_OK : ESP_FAIL;
    // result += application_task_initialize_data();

    return result;
}

/**
 * @brief Main execution function for the temperature monitor.
 *
 * This function is responsible for executing the temperature monitor's tasks.
 * It continuously monitors the sensor and performs any required actions
 * such as reading the sensor data, logging the results, or triggering
 * events based on specific conditions.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void application_task_execute(void *pvParameters) {
    global_config = (global_config_st *)pvParameters;
    if ((application_task_initialize() != ESP_OK) || (global_config == NULL)) {
        ESP_LOGE(TAG, "Failed to initialize application task");
        vTaskDelete(NULL);
    }
    command_config_st command_config = {0};
    command_write_st command_write   = {0};
    response_read_st response_read   = {0};
    response_write_st response_write = {0};

    uuid_ut uid;
    uint8_t uid_len;

    command_config.block  = 1;
    command_config.sector = 1;
    command_config.mode   = READ;

    while (1) {
        if (command_config.mode == READ) {
            if (readPassiveTargetID(PN532_MIFARE_ISO14443A, uid.bytes, &uid_len, 1000)) {
                if (read_sector(uid.bytes, uid_len, command_config.sector, command_config.block, sizeof(response_read.data), response_read.data) == ESP_OK) {
                    response_read.sector       = command_config.sector;
                    response_read.block        = command_config.block;
                    response_read.uuid.integer = uid.integer;

                    BaseType_t result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_RESPONSE_READ].queue,
                                                   &response_read,
                                                   pdMS_TO_TICKS(100));
                    if (result != pdPASS) {
                        ESP_LOGW(TAG,
                                 "Failed to send %s data to queue",
                                 global_config->mqtt_topics[DATA_STRUCT_RESPONSE_READ].topic);
                    }
                } else {
                    ESP_LOGW(TAG, "Failed to read data to Sector %d Block %d.\n", command_config.sector, command_config.block);
                }
            }
        } else if (command_config.mode == WRITE) {
            int8_t status = 0;

            if (xQueueReceive(global_config->mqtt_topics[DATA_STRUCT_COMMAND_WRITE].queue, &command_write, pdMS_TO_TICKS(100))) {
                if (write_sector(uid.bytes, uid_len, command_write.sector, command_write.block, command_write.data) != ESP_OK) {
                    status = -1;
                    ESP_LOGW(TAG, "Failed to write data to Sector %d Block %d.\n", command_config.sector, command_config.block);
                }

                response_write.status       = status;
                response_write.block        = command_write.block;
                response_write.sector       = command_write.sector;
                response_write.uuid.integer = uid.integer;

                BaseType_t result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_RESPONSE_WRITE].queue,
                                               &response_write,
                                               pdMS_TO_TICKS(100));
                if (result != pdPASS) {
                    ESP_LOGW(TAG,
                             "Failed to send %s data to queue",
                             global_config->mqtt_topics[DATA_STRUCT_RESPONSE_WRITE].topic);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
