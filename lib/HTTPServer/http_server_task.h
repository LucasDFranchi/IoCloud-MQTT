#ifndef HTTP_SERVER_TASK_H
#define HTTP_SERVER_TASK_H

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * @file http_server.h
 * @brief HTTP server interface for handling web requests on the ESP32.
 */

/**
 * @brief Main execution function for the HTTP server.
 *
 * This function initializes and starts the HTTP server, enabling the ESP32 to
 * handle incoming web requests. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void http_server_task_execute(void *pvParameters);

#endif /* HTTP_SERVER_TASK_H */
