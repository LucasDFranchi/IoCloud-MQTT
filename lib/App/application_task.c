#include "application_task.h"
#include "Driver/pn532.h"
#include "application_external_types.h"
#include "logger.h"
#include "global_config.h"

#include "esp_err.h"

#define TAG_UUID_MAX_SIZE (7)
#define MIFARE_BLOCKS_PER_SECTOR (4)
#define MIFARE_DATA_BLOCKS_PER_SECTOR (3)
#define MIFARE_TRAILER_BLOCK (3)
#define MIFARE_BLOCK_SIZE (16)
#define MIFARE_SECTOR_SIZE (MIFARE_BLOCK_SIZE * MIFARE_BLOCKS_PER_SECTOR)

#define MIFARE_INVALID_BLOCK (0x201)
#define MIFARE_AUTH_ERROR (0x202)
#define MIFARE_WRITE_ERROR (0x203)
#define MIFARE_INVALID_BLOCK_SIZE (0x205)
#define MIFARE_READ_ERROR (0x206)

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
    uint8_t block_offset = sector * MIFARE_BLOCKS_PER_SECTOR;
    uint8_t block_index  = block_offset + block;

    if (block >= MIFARE_DATA_BLOCKS_PER_SECTOR) {
        logger_print(ERR, TAG, "%s - Block %d is out of range for Sector %d.\n",
                 __func__,
                 block,
                 sector);
        return MIFARE_INVALID_BLOCK;
    }

    if (!uid || !data) {
        logger_print(ERR, TAG, "%s - Invalid input: UID or data buffer is NULL.",
                 __func__);
        return ESP_ERR_INVALID_ARG;
    }

    if (!mifareclassic_AuthenticateBlock(uid,
                                         uid_len,
                                         block_offset + MIFARE_TRAILER_BLOCK,
                                         USE_KEY_A,
                                         DEFAULT_KEY_A)) {
        logger_print(ERR, TAG, "%s - Authentication failed for Sector %d.\n",
                 __func__,
                 sector);
        return MIFARE_AUTH_ERROR;
    }

    if (!mifareclassic_WriteDataBlock(block_index, data)) {
        logger_print(ERR, TAG, "%s - Failed to write Block %d of Sector %d.\n",
                 __func__,
                 block,
                 sector);
        return MIFARE_WRITE_ERROR;
    }

    return ESP_OK;
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
 * @param[out] read_data_len Size of the output buffer in bytes. Must be at least
 *                      `MIFARE_BLOCK_SIZE` (16 bytes).
 * @param[out] read_data Pointer to the buffer where the read data will be stored.
 *
 * @return
 *  - ESP_OK on success.
 *  - ESP_ERR_INVALID_ARG if the inputs are invalid (e.g., NULL pointers, block out of range).
 *  - ESP_ERR_INVALID_SIZE if the output buffer size is insufficient.
 *  - ESP_FAIL on fail.
 */
esp_err_t read_sector(uint8_t *uid, uint8_t uid_len, uint8_t sector, uint8_t block, uint8_t read_data_len, uint8_t *read_data) {
    uint8_t block_offset = sector * 4;
    uint8_t block_index  = block_offset + block;

    if (!uid || !read_data) {
        logger_print(ERR, TAG, "%s - Invalid input: UID or data buffer is NULL.",
                 __func__);
        return ESP_ERR_INVALID_ARG;
    }

    if (block >= MIFARE_DATA_BLOCKS_PER_SECTOR) {
        logger_print(ERR, TAG, "%s - Block %d is out of range for Sector %d.",
                 __func__,
                 block,
                 sector);
        return MIFARE_INVALID_BLOCK;
    }

    if (read_data_len < MIFARE_BLOCK_SIZE) {
        logger_print(ERR, TAG, "%s - Output buffer size is too small for the data.", __func__);
        return MIFARE_INVALID_BLOCK_SIZE;
    }

    if (!mifareclassic_AuthenticateBlock(uid,
                                         uid_len,
                                         block_offset + MIFARE_TRAILER_BLOCK,
                                         USE_KEY_A,
                                         DEFAULT_KEY_A)) {
        logger_print(ERR, TAG, "%s - Authentication failed for Sector %d.",
                 __func__,
                 sector);
        return MIFARE_AUTH_ERROR;
    }

    if (!mifareclassic_ReadDataBlock(block_index, read_data)) {
        logger_print(ERR, TAG, "%s - Failed to read Block %d of Sector %d.",
                 __func__,
                 block,
                 sector);
        return MIFARE_READ_ERROR;
    }

    return ESP_OK;
}

/**
 * @brief Checks if a tag is present in the field and reads its UID.
 *
 * @param uid Pointer to store the UID of the detected tag.
 * @param uid_len Pointer to store the length of the UID.
 * @return true if a tag is detected and UID is read, false otherwise.
 */
static bool has_tag_in_field(uuid_ut *uid, uint8_t *uid_len) {
    if (!uid || !uid_len) {
        return false;
    }
    return readPassiveTargetID(PN532_MIFARE_ISO14443A, uid->bytes, uid_len, 1000);
}

/**
 * @brief Attempts to fetch a new configuration from the queue.
 *
 * This function checks if there is a new configuration available in the
 * queue and retrieves it if present.
 *
 * @param command_config Pointer to store the fetched configuration.
 * @return true if a new configuration was retrieved, false otherwise.
 */void try_fetch_new_config(command_config_st *command_config) {
    if (command_config == NULL) {
        return;
    }

    BaseType_t is_data_in_queue =
        xQueueReceive(global_config->mqtt_topics[DATA_STRUCT_COMMAND_CONFIG].queue,
                      command_config,
                      pdMS_TO_TICKS(100));

    if (is_data_in_queue == pdTRUE) {
        logger_print(DEBUG, TAG, "%s - New configuration received: Sector %d, Block %d, Mode %d",
                 __func__,
                 command_config->sector,
                 command_config->block,
                 command_config->mode);
    }
}

/**
 * @brief Processes a write command by writing data to the specified sector and block.
 *
 * This function validates the input parameters, writes data to the specified sector/block,
 * updates the response structure, and sends the response to an MQTT queue.
 *
 * @param[in] uid Pointer to the UUID structure.
 * @param[in] uid_len Length of the UUID in bytes.
 * @param[in] command_write Pointer to the write command structure containing sector, block, and data.
 * @param[out] response_write Pointer to the response structure to be populated.
 *
 * @return
 *     - ESP_OK on success.
 *     - ESP_ERR_INVALID_ARG if any input pointer is NULL.
 *     - ESP_FAIL if writing to the sector/block fails.
 *     - ESP_ERR_TIMEOUT if sending the response to the queue fails.
 */
static esp_err_t process_command_write(uuid_ut *uid, size_t uid_len, command_write_st *command_write, response_write_st *response_write) {
    int8_t status = 0;

    if (!command_write || !response_write || !uid) {
        logger_print(ERR, TAG, "%s - Invalid input: command_write or response_write or uid is NULL.",
                 __func__);
        return ESP_ERR_INVALID_ARG;
    }

    if (write_sector(uid->bytes, uid_len, command_write->sector, command_write->block, command_write->data) != ESP_OK) {
        status = -1;
        logger_print(ERR, TAG, "%s - Failed to write data to Sector %d Block %d.",
                 __func__,
                 command_write->sector,
                 command_write->block);
    }

    response_write->status       = status;
    response_write->block        = command_write->block;
    response_write->sector       = command_write->sector;
    response_write->uuid.integer = uid->integer;

    BaseType_t result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_RESPONSE_WRITE].queue,
                                   response_write,
                                   pdMS_TO_TICKS(100));
    if (result != pdPASS) {
        logger_print(ERR, TAG,
                 "%s - Failed to send %s data to queue",
                 __func__,
                 global_config->mqtt_topics[DATA_STRUCT_RESPONSE_WRITE].topic);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

/**
 * @brief Processes a read response by retrieving data from a specified sector and block.
 *
 * This function reads data from the given sector and block, populates the response structure,
 * and sends the response to an MQTT queue.
 *
 * @param uid Pointer to the UUID structure.
 * @param uid_len Length of the UUID in bytes.
 * @param command_config Pointer to the command configuration structure containing sector and block info.
 * @param response_read Pointer to the response structure to be populated.
 *
 * @return
 *     - ESP_OK on success.
 *     - ESP_ERR_INVALID_ARG if any input pointer is NULL.
 *     - ESP_FAIL if reading the sector/block fails.
 *     - ESP_ERR_TIMEOUT if sending the response to the queue fails.
 */
static esp_err_t process_response_read(uuid_ut *uid, size_t uid_len, command_config_st *command_config, response_read_st *response_read) {
    if (!command_config || !response_read || !uid) {
        logger_print(ERR, TAG, "Invalid input: command_config or response_read or uid is NULL.");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t result = read_sector(uid->bytes,
                                   uid_len,
                                   command_config->sector,
                                   command_config->block,
                                   sizeof(response_read->data),
                                   response_read->data);

    if (result == ESP_OK) {
        response_read->sector       = command_config->sector;
        response_read->block        = command_config->block;
        response_read->uuid.integer = uid->integer;

        logger_print(DEBUG, TAG, "Data read from Sector %d Block %d: ", command_config->sector, command_config->block);
        for (int i = 0; i < sizeof(response_read->data); i++) {
            logger_print(DEBUG, TAG, "%02X ", (char)response_read->data[i]);
        }

        BaseType_t queue_result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_RESPONSE_READ].queue,
                                             response_read,
                                             pdMS_TO_TICKS(100));
        if (queue_result != pdPASS) {
            logger_print(ERR, TAG, " %s - Failed to send %s data to queue",
                     __func__,
                     global_config->mqtt_topics[DATA_STRUCT_RESPONSE_READ].topic);
            return ESP_ERR_TIMEOUT;
        }
    } else {
        logger_print(ERR, TAG, "%s - Failed to read data from Sector %d Block %d.",
                 __func__,
                 command_config->sector,
                 command_config->block);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Initializes the application hardware and peripherals.
 *
 * This function is responsible for setting up the necessary hardware components,
 * If the initialization fails, the function logs an error and may enter a wait loop.
 *
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t application_task_initialize(void) {
    esp_err_t result = ESP_OK;

    if (!init_PN532_I2C(GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_19, GPIO_NUM_18, I2C_NUM_0)) {
        logger_print(ERR, TAG, "Failed to initialize PN532");
        return ESP_FAIL;
    }

    uint32_t versiondata = getPN532FirmwareVersion();
    if (!versiondata) {
        logger_print(ERR, TAG, "Didn't find PN53x board - %ld", versiondata);
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    logger_print(DEBUG, TAG, "Found chip PN5 %ld", (versiondata >> 24) & 0xFF);
    logger_print(DEBUG, TAG, "Firmware ver. %ld.%ld", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);

    result = SAMConfig() ? ESP_OK : ESP_FAIL;

    return result;
}

/**
 * @brief Main application task loop.
 *
 * This task is responsible for continuously processing incoming commands and
 * managing responses based on the detected field conditions.
 *
 * @param pvParameters Pointer to the global configuration structure.
 */
void application_task_execute(void *pvParameters) {
    global_config = (global_config_st *)pvParameters;
    if ((application_task_initialize() != ESP_OK) || (global_config == NULL)) {
        logger_print(ERR, TAG, " %s - Failed to initialize application task", __func__);
        vTaskDelete(NULL);
    }
    command_write_st command_write   = {0};
    response_read_st response_read   = {0};
    response_write_st response_write = {0};
    command_config_st command_config = {
        .block  = 0,
        .sector = 1,
        .mode   = READ,
    };

    uuid_ut uid;
    uint8_t uid_len;

    while (1) {
        try_fetch_new_config(&command_config);

        if (has_tag_in_field(&uid, &uid_len)) {
            bool queue_result = xQueueReceive(global_config->mqtt_topics[DATA_STRUCT_COMMAND_WRITE].queue,
                                              &command_write,
                                              pdMS_TO_TICKS(100));
            if (queue_result == pdPASS) {
                process_command_write(&uid, uid_len, &command_write, &response_write);
            }

            process_response_read(&uid, uid_len, &command_config, &response_read);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
