#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include "events_definition.h"
#include "tasks_definition.h"

/**
 * @brief Configuration structure for the Access Point.
 */
typedef struct global_config_s {
    EventGroupHandle_t firmware_event_group;  ///< Event group for signaling system status and events.
    QueueHandle_t app_data_queue;             ///< External handle for the sensor data queue.
} global_config_st;

#endif /* GLOBAL_CONFIG_H */