#include "application_task.h"
#include "Driver/ADS1115.h"
#include "Driver/max6675.h"
#include "Driver/tca9548a.h"
#include "application_external_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "global_config.h"

#include <driver/i2c.h>
#include <math.h>

#define SDA_IO (21) /*!< gpio number for I2C master data  */
#define SCL_IO (22) /*!< gpio number for I2C master clock */

#define FREQ_HZ (100000)   /*!< I2C master clock frequency */
#define TX_BUF_DISABLE (0) /*!< I2C master doesn't need buffer */
#define RX_BUF_DISABLE (0) /*!< I2C master doesn't need buffer */

#define I2C_NUM I2C_NUM_0               /*!< I2C number */
#define I2C_MODE I2C_MODE_MASTER        /*!< I2C mode to act as */
#define I2C_RX_BUF_STATE RX_BUF_DISABLE /*!< I2C set rx buffer status */
#define I2C_TX_BUF_STATE TX_BUF_DISABLE /*!< I2C set tx buffer status */
#define I2C_INTR_ALOC_FLAG (0)          /*!< I2C set interrupt allocation flag */

/* i2c setup ----------------------------------------- */
// Config profile for espressif I2C lib
i2c_config_t i2c_cfg = {
    .mode             = I2C_MODE_MASTER,
    .sda_io_num       = SDA_IO,
    .scl_io_num       = SCL_IO,
    .sda_pullup_en    = GPIO_PULLUP_DISABLE,
    .scl_pullup_en    = GPIO_PULLUP_DISABLE,
    .master.clk_speed = FREQ_HZ,
};

/* ADS1115 setup ------------------------------------- */
// Below uses the default values speficied by the datasheet
ads1115_t ads1115_cfg = {
    .reg_cfg = ADS1115_CFG_LS_COMP_MODE_TRAD |  // Comparator is traditional
               ADS1115_CFG_LS_COMP_LAT_NON |    // Comparator is non-latching
               ADS1115_CFG_LS_COMP_POL_LOW |    // Alert is active low
               ADS1115_CFG_LS_COMP_QUE_DIS |    // Compator is disabled
               ADS1115_CFG_LS_DR_1600SPS |      // No. of samples to take
               ADS1115_CFG_MS_PGA_FSR_4_096V,   // Mode is set to single-shot
    .dev_addr = 0x48,
};

/* TCA9548A setup ------------------------------------- */
static const tca9548a_t tca9548a_cfg = {
    .port_num = I2C_NUM_0,
    .dev_addr = 0x70,
};

/**
 * @file Application.c
 * @brief Temperature monitoring and logging for temperature and humidity data.
 *
 * This module interfaces with the AHT10 temperature and humidity sensor,
 * reads the data, and logs the temperature and humidity values periodically.
 * It provides functions to initialize the sensor, execute continuous readings,
 * and log the results.
 */

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_config_st *global_config = NULL;  ///< Global configuration structure.

static const char *TAG = "Application Task";  ///< Tag used for logging.

static esp_err_t send_data_modbus(float temperature) {
    uint8_t tx_buffer[256] = {0};  // Buffer to hold data to be sent
    extern QueueHandle_t uart_transmit_queue;  // Queue for UART transmission

    int len = snprintf((char *)tx_buffer, sizeof(tx_buffer), "Temperature : %.2f ºC", temperature);

    if (len < 0 || len >= sizeof(tx_buffer)) {
        ESP_LOGE(TAG, "Failed to format data for transmission");
        return ESP_FAIL;
    }

    if (xQueueSend(uart_transmit_queue, tx_buffer, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send data to UART queue");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Initializes the application hardware and peripherals.
 *
 * This function is responsible for setting up the necessary hardware components,
 * If the initialization fails, the function logs an error and may enter a wait loop.
 *
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t application_task_initialize(void) {
    i2c_param_config(I2C_NUM, &i2c_cfg);
    i2c_driver_install(I2C_NUM, I2C_MODE, I2C_RX_BUF_STATE, I2C_TX_BUF_STATE, I2C_INTR_ALOC_FLAG);

    // Setup ADS1115
    ADS1115_initiate(&ads1115_cfg);

    tca9548a_initialize(&tca9548a_cfg, GPIO_NUM_21, GPIO_NUM_22);

    return ESP_OK;
}

#define R0 10000    // Reference resistance at 25°C (in ohms)
#define BETA 3950   // Beta value (in Kelvin)
#define T0 298.15   // 25°C in Kelvin (273.15 + 25)
#define RREF 10000  // Reference resistor value (in ohms)

/**
 * @brief Main application task loop.
 *
 * This task is responsible for continuously processing incoming commands and
 * managing responses based on the detected field conditions.
 *
 * @param pvParameters Pointer to the global configuration structure.
 */
void application_task_execute(void *pvParameters) {
    global_config = (global_config_st *)pvParameters;
    if ((application_task_initialize() != ESP_OK) || (global_config == NULL)) {
        ESP_LOGE(TAG, " %s - Failed to initialize application task", __func__);
        vTaskDelete(NULL);
    }
    temperature_config_st temperature_config = {
        .time_interval = 5000,
    };

    tca9548a_set_channel(&tca9548a_cfg, 2);  // Select channel 0 for the temperature sensor

    while (1) {
        // Request single ended on pin AIN0
        ADS1115_request_single_ended_AIN1();  // all functions except for get_conversion_X return 'esp_err_t' for logging

        // Return latest conversion value
        uint16_t raw_value = 0;
        raw_value = ADS1115_get_conversion();
        // float voltage = raw_value * (4.095 / 32768.0);  // Scale raw value to voltage
        ESP_LOGI(TAG, "Raw Value: %d", raw_value);

        // Convert the raw ADC value to voltage
        float voltage = raw_value * (4.096 / 32768.0);  // Adjust based on your reference voltage
        ESP_LOGI(TAG, "Conversion Value: %f", voltage);

        // Convert the voltage to resistance using the voltage divider formula
        float resistance = RREF * (voltage / (3.3 - voltage));  // Using 3.3V as the input voltage

        // Use the Beta equation to calculate temperature in Kelvin
        float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(resistance / R0));

        // Convert Kelvin to Celsius
        float temperatureC = tempK - 273.15;
        ESP_LOGI(TAG, "Temperature Value: %0.2f", temperatureC);

        send_data_modbus(temperatureC);

        temperature_response_st temperature_response = {
            .internal_temperature = 25,
            .humidity             = 50,
            .temperature_array    = 0,
        };

        temperature_response.temperature_array = temperatureC;

        BaseType_t queue_result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_TEMPERATURE_RESPONSE].queue,
                                             &temperature_response,
                                             pdMS_TO_TICKS(100));

        if (queue_result != pdPASS) {
            ESP_LOGW(TAG, " %s - Failed to send %s data to queue",
                     __func__,
                     global_config->mqtt_topics[DATA_STRUCT_TEMPERATURE_RESPONSE].topic);
        }

        ADS1115_request_single_ended_AIN2();  // Request differential measurement on AIN2 and AIN3
        raw_value = ADS1115_get_conversion();
        voltage = raw_value * (4.095 / 32768.0);  // Scale raw value to voltage
        ESP_LOGI(TAG, "Float Value: %f", voltage);
        vTaskDelay(pdMS_TO_TICKS(temperature_config.time_interval));
    }
}
