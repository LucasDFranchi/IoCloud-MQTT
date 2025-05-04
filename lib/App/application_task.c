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

typedef enum sensor_channel_e {
    MUX_CHANNEL_0 = 0,
    MUX_CHANNEL_1,
    MUX_CHANNEL_2,
    MUX_CHANNEL_3,
    MUX_CHANNEL_4,
    MUX_CHANNEL_5,
    MUX_CHANNEL_6,
    MUX_CHANNEL_7,
} mux_channel_et;

typedef enum adc_configuration_e {
    ADC_CONFIG_SINGLE_ENDED_A0 = 0,
    ADC_CONFIG_SINGLE_ENDED_A1,
    ADC_CONFIG_SINGLE_ENDED_A2,
    ADC_CONFIG_SINGLE_ENDED_A3,
    ADC_CONFIG_DIFF_A0_A1,
    ADC_CONFIG_DIFF_A0_A2,
    ADC_CONFIG_DIFF_A0_A3,
    ADC_CONFIG_DIFF_A1_A2,
    ADC_CONFIG_DIFF_A1_A3,
    ADC_CONFIG_DIFF_A2_A3,
} adc_configuration_et;

typedef struct sensor_info_s {
    sensor_type_et type;             /*!< Type of sensor */
    mux_channel_et channel;          /*!< Channel number for the sensor */
    adc_configuration_et adc_config; /*!< ADC configuration type */
} sensor_info_st;

static const sensor_info_st sensor_info[NUM_OF_CHANNELS] = {
    {SENSOR_TYPE_TEMPERATURE, MUX_CHANNEL_2, ADC_CONFIG_SINGLE_ENDED_A1},  // Channel 0 for temperature sensor
    {SENSOR_TYPE_HART, MUX_CHANNEL_2, ADC_CONFIG_DIFF_A2_A3},              // Channel 1 for HART sensor
};

sensor_response_st sensor_response = {0};  ///< Sensor response structure to hold sensor data.

ads1115_config_st ads1115_config = {
    .dev_addr      = 0x48,
    .reg_cfg.value = 0,
};

tca9548a_config_st tca9548a_cfg = {
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

// static esp_err_t send_data_modbus(float temperature) {
//     uint8_t tx_buffer[256] = {0};              // Buffer to hold data to be sent
//     extern QueueHandle_t uart_transmit_queue;  // Queue for UART transmission

//     int len = snprintf((char *)tx_buffer, sizeof(tx_buffer), "Temperature : %.2f ºC", temperature);

//     if (len < 0 || len >= sizeof(tx_buffer)) {
//         ESP_LOGE(TAG, "Failed to format data for transmission");
//         return ESP_FAIL;
//     }

//     if (xQueueSend(uart_transmit_queue, tx_buffer, portMAX_DELAY) != pdPASS) {
//         ESP_LOGE(TAG, "Failed to send data to UART queue");
//         return ESP_FAIL;
//     }

//     return ESP_OK;
// }

/**
 * @brief Initializes the application hardware and peripherals.
 *
 * This function is responsible for setting up the necessary hardware components,
 * If the initialization fails, the function logs an error and may enter a wait loop.
 *
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t application_task_initialize(void) {
    i2c_config_t i2c_cfg = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = SDA_IO,
        .scl_io_num       = SCL_IO,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = FREQ_HZ,
    };

    i2c_param_config(I2C_NUM, &i2c_cfg);
    i2c_driver_install(I2C_NUM, I2C_MODE, I2C_RX_BUF_STATE, I2C_TX_BUF_STATE, I2C_INTR_ALOC_FLAG);

    ads1115_config.reg_cfg.bits.comp_que  = COMP_QUE_DISABLE;
    ads1115_config.reg_cfg.bits.comp_lat  = COMP_LAT_NON_LATCHING;
    ads1115_config.reg_cfg.bits.comp_pol  = COMP_POL_ACTIVE_LOW;
    ads1115_config.reg_cfg.bits.comp_mode = COMP_MODE_TRADITIONAL;
    ads1115_config.reg_cfg.bits.dr        = DR_128SPS;
    ads1115_config.reg_cfg.bits.mode      = MODE_SINGLESHOT;
    ads1115_config.reg_cfg.bits.pga       = PGA_4_096V;
    ads1115_config.reg_cfg.bits.mux       = MUX_AIN0_AIN1;
    ads1115_config.reg_cfg.bits.os        = OS_NO_EFFECT;
    ADS1115_initialize(&ads1115_config);

    tca9548a_initialize(&tca9548a_cfg);

    return ESP_OK;
}

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

    static mux_channel_et last_mux_channel = 0;  // Variable to keep track of the last selected channel

    while (1) {
        for (int i = 0; i < NUM_OF_CHANNELS; i++) {
            if (sensor_info[i].channel != last_mux_channel) {
                tca9548a_set_channel(sensor_info[i].channel);
                last_mux_channel = sensor_info[i].channel;
                vTaskDelay(pdMS_TO_TICKS(10));  // Wait for the channel to stabilize
                ESP_LOGI(TAG, "Switched to channel %d", sensor_info[i].channel);
            }

            switch (sensor_info[i].adc_config) {
                case ADC_CONFIG_SINGLE_ENDED_A0:
                    ADS1115_set_mux(MUX_AIN0_GND);
                    ADS1115_set_os(OS_START_SINGLE_CONV);
                    ADS1115_update();
                    // ESP_LOGI(TAG, "Requesting single-ended measurement on AIN0");
                    break;
                case ADC_CONFIG_SINGLE_ENDED_A1:
                    ADS1115_set_mux(MUX_AIN1_GND);
                    ADS1115_set_os(OS_START_SINGLE_CONV);
                    ADS1115_update();
                    // ESP_LOGI(TAG, "Requesting single-ended measurement on AIN1");
                    break;
                case ADC_CONFIG_SINGLE_ENDED_A2:
                    ADS1115_set_mux(MUX_AIN2_GND);
                    ADS1115_set_os(OS_START_SINGLE_CONV);
                    ADS1115_update();
                    // ESP_LOGI(TAG, "Requesting single-ended measurement on AIN2");
                    break;
                case ADC_CONFIG_SINGLE_ENDED_A3:
                    ADS1115_set_mux(MUX_AIN3_GND);
                    ADS1115_set_os(OS_START_SINGLE_CONV);
                    ADS1115_update();
                    // ESP_LOGI(TAG, "Requesting single-ended measurement on AIN3");
                    break;
                case ADC_CONFIG_DIFF_A0_A1:
                    ADS1115_set_mux(MUX_AIN0_AIN1);
                    ADS1115_set_os(OS_START_SINGLE_CONV);
                    ADS1115_update();
                    break;
                case ADC_CONFIG_DIFF_A0_A3:
                    ADS1115_set_mux(MUX_AIN0_AIN3);
                    ADS1115_set_os(OS_START_SINGLE_CONV);
                    ADS1115_update();
                    break;
                case ADC_CONFIG_DIFF_A2_A3:
                    ADS1115_set_mux(MUX_AIN2_AIN3);
                    ADS1115_set_os(OS_START_SINGLE_CONV);
                    ADS1115_update();
                    break;
                default:
                    ESP_LOGE(TAG, "Invalid ADC configuration for channel %d", sensor_info[i].channel);
                    break;
            }

            while (ADS1115_get_conversion_state() == false) {
                vTaskDelay(pdMS_TO_TICKS(100));  // Wait for the conversion to complete
            }

            uint16_t raw_value = ADS1115_get_raw_value();
            ESP_LOGI(TAG, "Raw Value[%d]: %d", i, raw_value);

            sensor_response.sensor_array[i].type      = sensor_info[i].type;  // Set the sensor type
            sensor_response.sensor_array[i].raw_value = raw_value;            // Set the raw value from the sensor
            sensor_response.num_of_active_sensors     = (i + 1);              // Set the number of active sensors
        }
        BaseType_t queue_result = xQueueSend(global_config->mqtt_topics[DATA_STRUCT_SENSOR_READ].queue,
                                             &sensor_response,
                                             pdMS_TO_TICKS(100));

        if (queue_result != pdPASS) {
            ESP_LOGW(TAG, " %s - Failed to send %s data to queue",
                     __func__,
                     global_config->mqtt_topics[DATA_STRUCT_SENSOR_READ].topic);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Wait for the specified time interval
    }
}
