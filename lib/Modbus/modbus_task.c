#include "driver/gpio.h"
#include <driver/uart.h>

#include "esp_err.h"
#include "esp_log.h"
#include "string.h"

#include "freertos/queue.h"

#define MODBUS_MAXIMUM_PACKET_SIZE 256  // Maximum packet size for Modbus RTU

static const uint8_t QUEUE_LENGTH          = 10;             // How many messages you can store
static const uint8_t UART_EVENT_QUEUE_SIZE = 30;             // Size of the UART event queue
static const char* TAG                     = "Modbus Task";  // Tag for logging

QueueHandle_t uart_event_queue    = {0};
QueueHandle_t uart_transmit_queue = {0};

typedef enum uart_direction_e {
    UART_RECEIVE_DATA = 0,
    UART_TRANSMIT_DATA,
} uart_direction_et;

static esp_err_t uart_init(void) {
    esp_err_t ret = ESP_FAIL;

    uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ret = uart_param_config(UART_NUM_2, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_driver_install(UART_NUM_2, 1024 * 2, 1024 * 2, UART_EVENT_QUEUE_SIZE, &uart_event_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    uart_transmit_queue = xQueueCreate(QUEUE_LENGTH, MODBUS_MAXIMUM_PACKET_SIZE);
    if (uart_transmit_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create UART transmit queue");
        uart_driver_delete(UART_NUM_2);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t set_direction_pin(uart_direction_et direction) {
    return gpio_set_level(GPIO_NUM_4, direction);
}

static void transmit_data(uint8_t* buf, uint16_t size) {
    if (buf == NULL || size == 0) {
        ESP_LOGE(TAG, "Invalid data to send");
        return;
    }

    set_direction_pin(UART_TRANSMIT_DATA);
    uart_write_bytes(UART_NUM_2, buf, size);
    printf("Transmitted: %.*s\n", size, buf);
    uart_wait_tx_done(UART_NUM_2, portMAX_DELAY);
    set_direction_pin(UART_RECEIVE_DATA);
}

static esp_err_t modbus_task_initialize(int gpio_direction) {
    esp_err_t ret = ESP_FAIL;

    ret = uart_init();
    if (ret != ESP_OK) {
        ESP_LOGE("MAX485", "Failed to initialize UART: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_reset_pin(gpio_direction);
    if (ret != ESP_OK) {
        ESP_LOGE("MAX485", "Failed to reset GPIO pin: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_set_direction(gpio_direction, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE("MAX485", "Failed to set GPIO direction: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

void modbus_task_execute(void* pvParameters) {
    uint8_t tx_buffer[MODBUS_MAXIMUM_PACKET_SIZE] = {0};
    uint8_t rx_buffer[MODBUS_MAXIMUM_PACKET_SIZE] = {0};
    uart_event_t event                            = {0};

    if (modbus_task_initialize(GPIO_NUM_4)) {
        ESP_LOGE("MAX485", "Failed to initialize MAX485 task");
        vTaskDelete(NULL);
    }

    while (1) {
        if (xQueueReceive(uart_event_queue, (void*)&event, 0) == pdTRUE) {
            switch (event.type) {
                case UART_DATA:
                if (event.size <= sizeof(rx_buffer)) {
                    ESP_LOGI(TAG, "Received %d bytes", event.size);
                } else {
                    ESP_LOGE(TAG, "Received data exceeds buffer size");
                    break;
                }
                
                uart_read_bytes(UART_NUM_2, rx_buffer, event.size, portMAX_DELAY);
                ESP_LOGI(TAG, "Received : %.*s", event.size, rx_buffer);
                memset(rx_buffer, 0, sizeof(rx_buffer));
                break;
                case UART_FRAME_ERR:
                ESP_LOGE(TAG, "UART_FRAME_ERR");
                break;
                default:
                ESP_LOGW(TAG, "Unhandled UART event: %d", event.type);
                break;
            }
        }
        
        if (xQueueReceive(uart_transmit_queue, tx_buffer, 0) == pdPASS) {
            ESP_LOGI(TAG, "Starting TX");
            transmit_data(tx_buffer, sizeof(tx_buffer));
            memset(tx_buffer, 0, sizeof(tx_buffer));
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Delay to prevent busy waiting
    }
}
