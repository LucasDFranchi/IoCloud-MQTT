/**
 * @file task_definition.h
 * @brief Task configuration definitions for the application.
 *
 * This header file contains the configuration details for various tasks
 * used in the application, including task priorities, stack sizes, names,
 * and default delays. These macros centralize task configuration to
 * improve readability and maintainability of the codebase.
 *
 * Task Descriptions:
 * - **Network Task**: Handles network-related operations, such as
 *   connectivity management and data transmission.
 * - **HTTP Server Task**: Runs the HTTP server to handle incoming client
 *   requests and serve responses.
 * - **Temperature Task**: Monitors and processes temperature sensor data
 *   periodically.
 * - **MQTT Task**: Manages MQTT client operations, including connecting
 *   to the broker, subscribing, and publishing messages.
 * - **SNTP Task**: Synchronizes the system time with an SNTP server.
 *
 * Note: Modify the priorities and stack sizes as needed based on the
 * task execution requirements and system constraints.
 */

#ifndef TASK_DEFINITION_H
#define TASK_DEFINITION_H

// Network Task configuration
#define NETWORK_TASK_PRIORITY 2
#define NETWORK_TASK_STACK_SIZE (2048 * 10)
#define NETWORK_TASK_NAME "Network Task"
#define NETWORK_TASK_DELAY 1000  // Delay in milliseconds

// HTTP Server Task configuration
#define HTTP_SERVER_TASK_PRIORITY 4
#define HTTP_SERVER_TASK_STACK_SIZE (2048 * 10)
#define HTTP_SERVER_TASK_NAME "HTTP Server Task"
#define HTTP_SERVER_TASK_DELAY 1000  // Delay in milliseconds

// Temperature Monitoring Task configuration
#define APPLICATION_TASK_PRIORITY 1
#define APPLICATION_TASK_STACK_SIZE (2048 * 10)
#define APPLICATION_TASK_NAME "Temperature Task"
#define APPLICATION_TASK_DELAY 1000  // Delay in milliseconds

// MQTT Client Task configuration
#define MQTT_CLIENT_TASK_PRIORITY 3
#define MQTT_CLIENT_TASK_STACK_SIZE (2048 * 10)
#define MQTT_CLIENT_TASK_NAME "MQTT Task"
#define MQTT_CLIENT_TASK_DELAY 1000  // Delay in milliseconds

// SNTP Task configuration
#define SNTP_TASK_PRIORITY 5
#define SNTP_TASK_STACK_SIZE (2048 * 10)
#define SNTP_TASK_NAME "SNTP Task"
#define SNTP_TASK_DELAY 1000  // Delay in milliseconds

// Watchdog Task configuration
#define WATCHDOG_TASK_PRIORITY 5
#define WATCHDOG_TASK_STACK_SIZE (2048 * 2)
#define WATCHDOG_TASK_NAME "Watchdog Task"
#define WATCHDOG_TASK_DELAY 5000  // Delay in milliseconds

#endif /* TASK_DEFINITION_H */
