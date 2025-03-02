#include "logger.h"
#include "global_config.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LOGGER_MAX_MSG_HEADER_LEN (64)                                               ///< Message Header 128 bytes
#define LOGGER_MAX_MSG_BODY_LEN (256)                                                ///< Message Body 896 bytes
#define LOGGER_MAX_PACKET_LEN (LOGGER_MAX_MSG_HEADER_LEN + LOGGER_MAX_MSG_BODY_LEN)  ///< Maximum Packet 1024 bytes
#define LOGGER_UDP_HOST "logs5.papertrailapp.com"                                    ///< Papertrail hostname
#define LOGGER_UDP_PORT (20770)                                                      ///< Papertrail port

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st* global_config = NULL;

static SemaphoreHandle_t logger_mutex = NULL;   ///< Mutex used for ensuring thread safety during UDP packet send operations.
static struct sockaddr_in dest_addr   = {0};    ///< Destination address structure for the UDP server.
static int sock                       = -1;     ///< UDP socket descriptor used for sending data.

/**
 * @brief Sends a UDP packet to the specified destination.
 *
 * This function sends the provided packet to the destination address using the
 * previously opened UDP socket. The function ensures thread safety by using a
 * semaphore to prevent concurrent access to the socket. If the packet or the socket
 * is invalid, the function returns an error.
 *
 * @param packet The packet to send.
 * @return ESP_OK on success, ESP_FAIL or other error codes on failure.
 */
static esp_err_t send_udp_packet(const char* packet) {
    if (sock < 0) {
        return ESP_FAIL;
    }

    if (packet == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (logger_mutex && xSemaphoreTake(logger_mutex, portMAX_DELAY)) {
        int sent = sendto(sock, packet, strlen(packet), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        xSemaphoreGive(logger_mutex);

        if (sent < 0) {
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

/**
 * @brief Sends a log message to the serial console.
 *
 * This function prints the provided log message to the standard output (console).
 * If the input packet is NULL, it returns an error.
 *
 * @param packet Pointer to the log message string.
 * @return ESP_OK on success, ESP_FAIL if the packet is NULL.
 */
static esp_err_t send_serial_packet(const char* packet) {
    if (packet == NULL) {
        return ESP_FAIL;
    }
    if (logger_mutex && xSemaphoreTake(logger_mutex, portMAX_DELAY)) {
        printf("%s\n", packet);
        xSemaphoreGive(logger_mutex);
    }
    return ESP_OK;
}

/**
 * @brief Checks if the device is connected to a Wi-Fi station.
 *
 * @return True if connected, false otherwise.
 */
static bool is_station_connected(void) {
    if (!global_config || !global_config->firmware_event_group) {
        return false;
    }

    EventBits_t firmware_event_bits = xEventGroupGetBits(global_config->firmware_event_group);

    return (firmware_event_bits & WIFI_CONNECTED_STA) == 1;
}

/**
 * @brief Opens a UDP socket and resolves the destination hostname to an IP address.
 *
 * This function initializes a UDP socket for logging, resolving the provided hostname
 * dynamically to support non-static IPs (e.g., Papertrail servers). If the hostname
 * resolution or socket creation fails, it returns an error.
 *
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t open_udp_socket(void) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family       = AF_INET;
    hints.ai_socktype     = SOCK_DGRAM;

    // Resolve hostname to IP
    if (getaddrinfo(LOGGER_UDP_HOST, NULL, &hints, &res) != 0) {
        return ESP_FAIL;
    }

    if (res == NULL) {
        return ESP_FAIL;
    }

    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    dest_addr.sin_addr       = addr->sin_addr;
    dest_addr.sin_family     = AF_INET;
    dest_addr.sin_port       = htons(LOGGER_UDP_PORT);

    freeaddrinfo(res);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Sends a formatted log message to the appropriate output (serial or UDP).
 *
 * This function formats a log message with the provided log level, tag, and message. It checks if the
 * device is connected to the station (network) and if the network is initialized. Based on this check,
 * the message is either sent via UDP (if the network is available) or serial (if not). If the message
 * exceeds the allowed size, an error is returned.
 *
 * @param level The severity level of the log message (e.g., "[INFO]", "[ERROR]").
 * @param tag The tag identifying the source of the log message.
 * @param message The content of the log message.
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_SIZE if the message size exceeds the limit,
 *         or an error code if network operations fail.
 */
static esp_err_t logger_send_message(const char* level, const char* tag, const char* message) {
    char final_message[LOGGER_MAX_PACKET_LEN];

    if (!level || !tag || !message) {
        return ESP_ERR_INVALID_ARG;
    }

    int message_size = snprintf(final_message, sizeof(final_message), "%s %s: %s", level, tag, message);
    if (message_size >= LOGGER_MAX_MSG_BODY_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (!is_station_connected() || !global_config->allow_external_logs) {
        return send_serial_packet(final_message);
    }

    esp_err_t result = send_udp_packet(final_message);

    if (result != ESP_OK) {
        if (open_udp_socket() != ESP_OK) {
            return send_serial_packet(final_message);
        }
    }

    return send_udp_packet(final_message);
}

/**
 * @brief Initializes the logger module.
 *
 * @param pvParameters Pointer to the global configuration.
 * @return ESP_OK on success, ESP_FAIL if initialization fails.
 */
esp_err_t logger_initialize(void* pvParameters) {
    global_config = (global_config_st*)pvParameters;
    if (global_config == NULL) {
        return ESP_FAIL;
    }

    if (logger_mutex == NULL) {
        logger_mutex = xSemaphoreCreateMutex();
        if (logger_mutex == NULL) {
            return ESP_FAIL;  // Failed to create the mutex
        }
    }

    return ESP_OK;
}

/**
 * @brief Prints a formatted log message based on the log level.
 *
 * This function formats a log message and sends it to the appropriate destination (UDP or serial)
 * based on the log level (INFO, WARN, ERROR, DEBUG). The message is first validated for size and
 * checked for proper initialization before being processed and sent.
 *
 * @param log_level The severity level of the log message (INFO, WARN, ERROR, DEBUG).
 * @param tag The tag identifying the source of the log message.
 * @param format A format string for the log message.
 * @param ... Variable arguments corresponding to the format string.
 *
 * @return ESP_OK on success, ESP_FAIL if initialization or arguments are invalid,
 *         ESP_ERR_INVALID_SIZE if the message size exceeds the limit,
 *         ESP_ERR_INVALID_ARG if an invalid log level is provided.
 */
esp_err_t logger_print(log_level_et log_level, const char* tag, const char* format, ...) {
    if ((!tag) || (!global_config) || !global_config->firmware_event_group) {
        return ESP_FAIL;
    }

    va_list args;
    va_start(args, format);
    char message_body[LOGGER_MAX_MSG_BODY_LEN] = {0};
    int body_size                              = vsnprintf(message_body, sizeof(message_body), format, args);
    va_end(args);

    if (body_size > LOGGER_MAX_MSG_BODY_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    switch (log_level) {
        case INFO:
            logger_send_message("[INFO]", tag, message_body);
            break;
        case WARN:
            logger_send_message("[WARN]", tag, message_body);
            break;
        case ERR:
            logger_send_message("[ERROR]", tag, message_body);
            break;
        case DEBUG:
            logger_send_message("[DEBUG]", tag, message_body);
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}
