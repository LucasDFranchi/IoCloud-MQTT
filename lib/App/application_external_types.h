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

// /**
//  * @enum data_types_e
//  * @brief Enumeration of supported data types for sensor data.
//  */
// typedef enum data_types_e {
//     DATA_TYPE_INT,   /**< Integer data type. */
//     DATA_TYPE_FLOAT, /**< Float data type. */
//     DATA_TYPE_ARRAY, /**< Array data type. */
// } data_types_et;

// /**
//  * @struct generic_sensor_data_st
//  * @brief Represents generic sensor data with a flexible type.
//  *
//  * This structure can hold sensor data of either integer or float type, determined
//  * by the `type` field. The `value` union allows storage of one type at a time.
//  */
// typedef struct generic_sensor_data_s {
//     data_types_et type; /**< The type of data stored in the union. */
//     union {
//         int int_val;        /**< Integer value when type is DATA_TYPE_INT. */
//         float float_val;    /**< Float value when type is DATA_TYPE_FLOAT. */
//         uint8_t *array_val; /**< Pointer to a dynamically allocated string when type is DATA_TYPE_STRING. */
//     } value;                /**< Union to hold the sensor data value. */
//     int num_elements;       /**< Optional: Number of elements in the array */
// } generic_sensor_data_st;

typedef enum data_struct_types_e {
    DATA_STRUCT_COMMAND_CONFIG = 0,
    DATA_STRUCT_COMMAND_WRITE,
    DATA_STRUCT_RESPONSE_READ,
    DATA_STRUCT_RESPONSE_WRITE,
    END_OF_DATA_STRUCT_TYPES,
} data_struct_types_et;

typedef enum data_direction_s {
    PUBLISH = 0,
    SUBSCRIBE,
} data_direction_st;

typedef union uuid_u {
    uint64_t integer;
    uint8_t bytes[8];
} uuid_ut;

typedef struct command_config_s {
    uint8_t block;
    uint8_t sector;
    uint8_t mode;
} command_config_st;

typedef struct command_write_s {
    uint8_t block;
    uint8_t sector;
    uint8_t data[16];
} command_write_st;

typedef struct response_read_s {
    uuid_ut uuid;
    uint8_t block;
    uint8_t sector;
    uint8_t data[16];
} response_read_st;

typedef struct response_write_s {
    uuid_ut uuid;
    uint8_t block;
    uint8_t sector;
    int8_t status;
} response_write_st;

typedef struct data_info_s {
    data_struct_types_et type;
    uint32_t size;
    data_direction_st direction;
} data_info_st;

#endif /* APPLICATION_EXTERNAL_TYPES_H */
