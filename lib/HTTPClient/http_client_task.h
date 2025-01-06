#ifndef HTTP_CLIENT_TASK_H
#define HTTP_CLIENT_TASK_H

#include "esp_err.h"
#include "esp_http_client.h"

/**
 * @file http_client.h
 * @brief HTTP client interface for handling web requests on the ESP32.
 */

/**
 * @brief Main execution function for the HTTP client.
 *
 * This function initializes and starts the HTTP client, enabling the ESP32 to
 * handle incoming web requests. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void http_client_task_execute(void *pvParameters);

#endif /* HTTP_CLIENT_TASK_H */

https://github.com/elechouse/PN532/blob/PN532_HSU/PN532_SPI/PN532_SPI.cpp
