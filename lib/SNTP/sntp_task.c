/**
 * @file sntp_task.c
 * @brief Implementation of SNTP synchronization task for ESP32.
 *
 * This file contains the implementation of a task that synchronizes the
 * system time using the Simple Network Time Protocol (SNTP). It includes
 * functions for initializing SNTP, retrieving time from an NTP server, and
 * signaling successful synchronization via an event group. This task ensures
 * that the system clock is accurately synchronized for time-dependent operations.
 */

#include "sntp_task.h"
#include "events_definition.h"
#include "tasks_definition.h"

#include "esp_log.h"
#include "lwip/apps/sntp.h"

/**
 * @brief Event group for signaling system status and events.
 *
 * This event group is used to communicate various system events and states between
 * different tasks. It provides flags that other tasks can check to determine the
 * status of Ethernet connection, IP acquisition, and other critical system states.
 * The event group helps in synchronizing events across tasks, enabling efficient
 * coordination of system activities.
 */
static EventGroupHandle_t *firmware_event_group = NULL;

static const char *TAG          = "SNTP Task";  ///< Tag for logging
static bool is_sntp_initialized = false;        ///< Tracks if SNTP has been initialized
static bool is_sntp_synced      = false;        ///< Tracks if time synchronization is successful

/**
 * @brief Initialize SNTP and attempt to synchronize time with the server.
 *
 * This function configures the SNTP client, initializes it if not already done, and
 * retrieves the current time. If the time is successfully synchronized, an event
 * bit (TIME_SYNCED) is set in the event group.
 */
static void sntp_task_sync_time_obtain_time(void) {
    time_t now         = 0;
    struct tm timeinfo = {0};

    time(&now);
    localtime_r(&now, &timeinfo);

    if (!is_sntp_initialized) {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();

        setenv("TZ", "GMT+3", 1);
        tzset();
        is_sntp_initialized = true;
    }

    if (timeinfo.tm_year < (2020 - 1900)) {
        xEventGroupClearBits(*firmware_event_group, TIME_SYNCED);
    } else {
        xEventGroupSetBits(*firmware_event_group, TIME_SYNCED);
        is_sntp_synced = true;
    }
}

/**
 * @brief Task to manage SNTP time synchronization.
 *
 * This task waits for the system to connect to the Wi-Fi network and then
 * attempts to synchronize the system time with an SNTP server. The task
 * exits once synchronization is successful.
 *
 * @param pvParameters Pointer to the event group handle for synchronization.
 */
void sntp_task_execute(void *pvParameters) {
    ESP_LOGI(TAG, "Starting SNTP task execution...");

    firmware_event_group = (EventGroupHandle_t *)pvParameters;
    if (firmware_event_group == NULL || *firmware_event_group == NULL) {
        ESP_LOGE(TAG, "Event group is not initialized.");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");
    EventBits_t firmware_event_bits = xEventGroupWaitBits(*firmware_event_group,
                                                          WIFI_CONNECTED_STA,
                                                          pdFALSE,
                                                          pdFALSE,
                                                          portMAX_DELAY);

    while (1) {
        if (firmware_event_bits & WIFI_CONNECTED_STA) {
            ESP_LOGI(TAG, "Trying to synchronize time...");
            sntp_task_sync_time_obtain_time();
        }

        vTaskDelay(pdMS_TO_TICKS(SNTP_TASK_DELAY));

        if (is_sntp_synced) {
            ESP_LOGI(TAG, "Time synchronization successful. Exiting SNTP task.");
            break;
        }
    }

    ESP_LOGI(TAG, "SNTP task completed. Deleting task...");
    vTaskDelete(NULL);
}