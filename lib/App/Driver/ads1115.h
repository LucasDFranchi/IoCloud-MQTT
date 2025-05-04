/*
 * @file ADS1115.h
 * @brief ADS1115 ADC driver implementation using ESP-IDF I2C APIs.
 *
 * This file provides functions for configuring and using the ADS1115 ADC
 * over I2C, including register-level control and data acquisition.
 *
 * @license Apache License 2.0
 * @author LucasD.Franchi@gmail.com
 */
#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @enum register_address_et
 * @brief Address pointer values for selecting internal registers.
 */
typedef enum {
    REG_ADDR_CONVERSION = 0b00, /**< Conversion register (00b) */
    REG_ADDR_CONFIG     = 0b01, /**< Config register     (01b) */
    REG_ADDR_LO_THRESH  = 0b10, /**< Lo_thresh register   (10b) */
    REG_ADDR_HI_THRESH  = 0b11  /**< Hi_thresh register   (11b) */
} register_address_et;

/**
 * @enum os_status_t
 * @brief Operational status or single-shot conversion control.
 */
typedef enum {
    OS_NO_EFFECT         = 0, /**< Write: No effect | Read: Conversion ongoing */
    OS_START_SINGLE_CONV = 1  /**< Write: Start conversion | Read: Idle */
} os_status_t;

/**
 * @enum mux_config_t
 * @brief Input multiplexer configuration (ADS1115 only).
 */
typedef enum {
    MUX_AIN0_AIN1 = 0b000, /**< AIN0 - AIN1 (default) */
    MUX_AIN0_AIN3 = 0b001, /**< AIN0 - AIN3 */
    MUX_AIN1_AIN3 = 0b010, /**< AIN1 - AIN3 */
    MUX_AIN2_AIN3 = 0b011, /**< AIN2 - AIN3 */
    MUX_AIN0_GND  = 0b100, /**< AIN0 - GND */
    MUX_AIN1_GND  = 0b101, /**< AIN1 - GND */
    MUX_AIN2_GND  = 0b110, /**< AIN2 - GND */
    MUX_AIN3_GND  = 0b111  /**< AIN3 - GND */
} mux_config_t;

/**
 * @enum pga_gain_et
 * @brief Programmable Gain Amplifier settings.
 */
typedef enum {
    PGA_6_144V   = 0b000, /**< ±6.144V (not on ADS1113) */
    PGA_4_096V   = 0b001, /**< ±4.096V */
    PGA_2_048V   = 0b010, /**< ±2.048V (default) */
    PGA_1_024V   = 0b011, /**< ±1.024V */
    PGA_0_512V   = 0b100, /**< ±0.512V */
    PGA_0_256V_1 = 0b101, /**< ±0.256V */
    PGA_0_256V_2 = 0b110, /**< ±0.256V */
    PGA_0_256V_3 = 0b111  /**< ±0.256V */
} pga_gain_et;

/**
 * @enum operating_mode_et
 * @brief Operating mode.
 */
typedef enum {
    MODE_CONTINUOUS = 0, /**< Continuous conversion */
    MODE_SINGLESHOT = 1  /**< Single-shot mode (default) */
} operating_mode_et;

/**
 * @enum data_rate_et
 * @brief Data rate (samples per second).
 */
typedef enum {
    DR_8SPS   = 0b000, /**< 8 SPS */
    DR_16SPS  = 0b001, /**< 16 SPS */
    DR_32SPS  = 0b010, /**< 32 SPS */
    DR_64SPS  = 0b011, /**< 64 SPS */
    DR_128SPS = 0b100, /**< 128 SPS (default) */
    DR_250SPS = 0b101, /**< 250 SPS */
    DR_475SPS = 0b110, /**< 475 SPS */
    DR_860SPS = 0b111  /**< 860 SPS */
} data_rate_et;

/**
 * @enum comparator_mode_t
 * @brief Comparator mode.
 */
typedef enum {
    COMP_MODE_TRADITIONAL = 0, /**< Traditional (default) */
    COMP_MODE_WINDOW      = 1  /**< Window */
} comparator_mode_t;

/**
 * @enum comparator_polarity_et
 * @brief Comparator polarity.
 */
typedef enum {
    COMP_POL_ACTIVE_LOW  = 0, /**< Active low (default) */
    COMP_POL_ACTIVE_HIGH = 1  /**< Active high */
} comparator_polarity_et;

/**
 * @enum comparator_latching_t
 * @brief Latching comparator mode.
 */
typedef enum {
    COMP_LAT_NON_LATCHING = 0, /**< Non-latching (default) */
    COMP_LAT_LATCHING     = 1  /**< Latching */
} comparator_latching_t;

/**
 * @enum comparator_queue_et
 * @brief Comparator queue/disable.
 */
typedef enum {
    COMP_QUE_ASSERT_1 = 0b00, /**< Assert after 1 conv */
    COMP_QUE_ASSERT_2 = 0b01, /**< Assert after 2 conv */
    COMP_QUE_ASSERT_4 = 0b10, /**< Assert after 4 conv */
    COMP_QUE_DISABLE  = 0b11  /**< Disable (default) */
} comparator_queue_et;

/**
 * @union register_config_ut
 * @brief 16-bit configuration register for ADS111x devices.
 */
typedef union register_config_u {
    /**
     * @struct register_config_u::bits
     * @brief Bit-field access to ADS111x configuration register.
     */
    struct {
        uint16_t comp_que : 2;  /**< Comparator queue/disable (COMP_QUE[1:0]) */
        uint16_t comp_lat : 1;  /**< Latching comparator */
        uint16_t comp_pol : 1;  /**< Comparator polarity (active low/high) */
        uint16_t comp_mode : 1; /**< Comparator mode (traditional/window) */
        uint16_t dr : 3;        /**< Data rate selection (DR[2:0]) */
        uint16_t mode : 1;      /**< Device operating mode (MODE) */
        uint16_t pga : 3;       /**< Programmable Gain Amplifier (PGA[2:0]) */
        uint16_t mux : 3;       /**< Input multiplexer selection (MUX[2:0]) */
        uint16_t os : 1;        /**< Operational status / start single conversion */
    } bits;

    uint16_t value; /**< Raw 16-bit register value */
} register_config_ut;

/**
 * @brief ADS1115 device configuration structure.
 *
 * This structure holds the configuration settings and I2C address for an ADS1115 device.
 * It is used to initialize the ADC and update its configuration registers.
 */
typedef struct ads1115_config_t {
    register_config_ut reg_cfg; /**< Configuration register bitfield (OS, MUX, PGA, etc.) */
    uint8_t dev_addr;           /**< 7-bit I2C address of the ADS1115 device */
} ads1115_config_st;

/**
 * @brief Set the operational status (start conversion).
 * @param os OS status (use @ref os_status_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_os(os_status_t os);

/**
 * @brief Set the input multiplexer configuration.
 * @param mux Mux setting (use @ref mux_config_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_mux(mux_config_t mux);

/**
 * @brief Set the programmable gain amplifier (PGA) value.
 * @param pga PGA setting (use @ref pga_gain_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_pga(pga_gain_et pga);

/**
 * @brief Set the ADC data rate.
 * @param rate Data rate (use @ref data_rate_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_data_rate(data_rate_et rate);

/**
 * @brief Set the ADC operating mode.
 * @param mode Operating mode (use @ref operating_mode_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_mode(operating_mode_et mode);

/**
 * @brief Set the comparator mode.
 * @param mode Mode (use @ref comparator_mode_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_mode(comparator_mode_t mode);

/**
 * @brief Set the comparator polarity.
 * @param pol Polarity (use @ref comparator_polarity_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_pol(comparator_polarity_et pol);

/**
 * @brief Set the comparator queue configuration.
 * @param que Comparator queue mode (use @ref comparator_queue_et).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_que(comparator_queue_et que);

/**
 * @brief Set the comparator latching mode.
 * @param lat Latching mode (use @ref comparator_latching_t).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 */
esp_err_t ADS1115_set_comp_lat(comparator_latching_t lat);

/**
 * @brief Write the current configuration to the ADS1115 device.
 *
 * Applies the internal configuration stored in `register_config`
 * to the ADS1115 config register via I2C.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ADS1115_update(void);

/**
 * @brief Initialize the ADS1115 device with provided configuration.
 *
 * Copies the device address and register configuration from the input structure,
 * applies settings, and writes to the device.
 *
 * @param dev Pointer to ADS1115 configuration structure
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if input is NULL
 */
esp_err_t ADS1115_initialize(ads1115_config_st *dev);

/**
 * @brief Read the current raw conversion result from the ADS1115.
 *
 * Initiates an I2C read from the conversion register.
 *
 * @return 16-bit raw ADC value
 */
uint16_t ADS1115_get_raw_value();

/**
 * @brief Check whether a conversion is currently complete.
 *
 * Reads the operational status (OS) bit in the config register to determine
 * if the conversion is complete.
 *
 * @return true if conversion is complete, false otherwise
 */
bool ADS1115_get_conversion_state();
