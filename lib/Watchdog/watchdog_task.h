#ifndef WATCHDOG_TASK_H
#define WATCHDOG_TASK_H

#include "esp_err.h"

/**
 * @file watchdog_task.c
 * @brief Implementation of a Task Watchdog Timer (TWDT) task for ESP32.
 *
 * This file contains the implementation of a watchdog task that ensures
 * system stability by monitoring task execution. It uses the ESP-IDF Task
 * Watchdog Timer (TWDT) to detect unresponsive tasks and trigger a system reset
 * if necessary. The watchdog task periodically resets the watchdog timer to
 * prevent unwanted resets while the system is functioning correctly.
 */

/**
 * @brief Task to monitor system stability using the Watchdog Timer.
 *
 * This task registers itself with the TWDT and continuously resets the watchdog
 * timer to indicate normal operation. If the task becomes unresponsive and fails
 * to reset the watchdog within the specified timeout, the system will reset.
 *
 * @param pvParameters Unused parameter (can be NULL).
 */
void watchdog_task_execute(void *pvParameters);

#endif  // WATCHDOG_TASK_H
