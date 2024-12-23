#ifndef SNTP_TASK_H
#define SNTP_TASK_H

#include "esp_err.h"

/**
 * @file sntp_task.h
 * @brief Interface for SNTP time synchronization task for ESP32.
 *
 * This header defines the interface for initializing and managing the SNTP
 * task, which synchronizes the system time with an NTP server and signals
 * successful synchronization through an event group.
 */

/**
 * @brief Task to manage SNTP time synchronization.
 *
 * This task waits for the system to connect to the Wi-Fi network and then
 * attempts to synchronize the system time with an SNTP server. The task
 * exits once synchronization is successful.
 *
 * @param pvParameters Pointer to the event group handle for synchronization.
 */
void sntp_task_execute(void *pvParameters);

#endif  // SNTP_TASK_H
