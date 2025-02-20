#ifndef LOGGER_H
#define LOGGER_H

#include "esp_err.h"

/**
 * @file logger.h
 * @brief Interface for logging functionality.
 *
 * This header defines the interface for initializing the logger and
 * sending log messages with different log levels (INFO, WARN, ERR, DEBUG).
 * The logger supports both serial and UDP logging, and the implementation
 * ensures that the appropriate logging method is used based on network status.
 */

/**
 * @enum log_level_e
 * @brief Enumeration of log levels.
 *
 * Defines the different log levels that can be used to categorize log messages.
 * - INFO: General informational messages.
 * - WARN: Warning messages indicating potential issues.
 * - ERR: Error messages indicating failure or critical issues.
 * - DEBUG: Detailed debug messages for troubleshooting.
 */
typedef enum log_level_e {
    INFO = 0,   /**< Informational messages */
    WARN,       /**< Warning messages */
    ERR,        /**< Error messages */
    DEBUG,      /**< Debug messages */
} log_level_et;

/**
 * @brief Initializes the logger.
 *
 * This function initializes the logger by setting up any necessary resources
 * for sending log messages. It prepares the logger to accept log messages
 * with different levels and routes them appropriately (e.g., to serial or UDP).
 *
 * @param pvParameters Pointer to any parameters that may be passed during
 *                      task initialization. This is typically used for
 *                      passing event group handles or other initialization
 *                      information.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t logger_initialize(void *pvParameters);

/**
 * @brief Prints a log message with a specified log level.
 *
 * This function formats the log message and sends it to the appropriate
 * logging mechanism based on the log level (INFO, WARN, ERR, DEBUG).
 * The message will be routed through serial or UDP, depending on the network
 * initialization state and connectivity.
 *
 * @param log_level The severity level of the log message (INFO, WARN, ERR, DEBUG).
 * @param tag A tag identifying the source of the log message.
 * @param format A format string for the log message, similar to printf.
 * @param ... Additional arguments to be inserted into the format string.
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if invalid arguments are provided,
 *         ESP_ERR_INVALID_SIZE if the log message exceeds the size limit, or
 *         other error codes based on the underlying send function.
 */
esp_err_t logger_print(log_level_et log_level, const char* tag, const char* format, ...);

#endif  // LOGGER_H
