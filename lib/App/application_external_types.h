#ifndef APPLICATION_EXTERNAL_TYPES_H
#define APPLICATION_EXTERNAL_TYPES_H

#include <stdint.h>

/**
 * @file application_external_types.h
 * @brief Definitions for commonly used data structures.
 *
 * This header file provides type definitions for data structures
 * used across the application. These structures are designed to
 * standardize the representation of various data elements, ensuring
 * consistency and facilitating communication between different
 * modules.
 */

 #define MAX_TEMPERATURE_ARRAY_SIZE (4)

typedef enum data_struct_types_e {
    DATA_STRUCT_TEMPERATURE_CONFIG = 0,
    DATA_STRUCT_TEMPERATURE_RESPONSE,
    END_OF_DATA_STRUCT_TYPES,
} data_struct_types_et;

typedef enum data_direction_s {
    PUBLISH = 0,
    SUBSCRIBE,
} data_direction_st;

typedef struct temperature_config_s {
    uint32_t time_interval;
} temperature_config_st;

typedef struct temperature_response_s {
    uint16_t temperature_array[MAX_TEMPERATURE_ARRAY_SIZE];
    uint16_t internal_temperature;
    uint16_t humidity;
} temperature_response_st;

typedef struct data_info_s {
    data_struct_types_et type;
    uint32_t size;
    data_direction_st direction;
} data_info_st;

#endif /* APPLICATION_EXTERNAL_TYPES_H */
