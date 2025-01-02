#ifndef APPLICATION_EXTERNAL_TYPES_H
#define APPLICATION_EXTERNAL_TYPES_H

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

/**
 * @enum data_types_e
 * @brief Enumeration of supported data types for sensor data.
 */
typedef enum data_types_e {
    DATA_TYPE_INT,   /**< Integer data type. */
    DATA_TYPE_FLOAT, /**< Float data type. */
} data_types_et;

/**
 * @struct generic_sensor_data_st
 * @brief Represents generic sensor data with a flexible type.
 *
 * This structure can hold sensor data of either integer or float type, determined
 * by the `type` field. The `value` union allows storage of one type at a time.
 */
typedef struct generic_sensor_data_st {
    data_types_et type; /**< The type of data stored in the union. */
    union {
        int int_val;     /**< Integer value when type is DATA_TYPE_INT. */
        float float_val; /**< Float value when type is DATA_TYPE_FLOAT. */
    } value;             /**< Union to hold the sensor data value. */
} generic_sensor_data_st;

#endif /* APPLICATION_EXTERNAL_TYPES_H */
