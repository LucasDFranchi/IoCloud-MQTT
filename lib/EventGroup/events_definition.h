#ifndef EVENTS_DEFINITION_H
#define EVENTS_DEFINITION_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/**
 * @brief FreeRTOS event group for signaling system events.
 *
 * This event group uses individual bits to signal different system events:
 * - WIFI_CONNECTED_STA: Indicates that the device has successfully connected to a network in station (STA) mode.
 * - WIFI_CONNECTED_AP: Indicates that the device has successfully established a network in access point (AP) mode.
 * - TIME_SYNCED: Indicates that the system time has been successfully synchronized with an external time source.
 *
 */
#define WIFI_CONNECTED_STA BIT0
#define WIFI_CONNECTED_AP BIT1
#define TIME_SYNCED BIT2

#endif /* EVENTS_DEFINITION_H */