#include "network.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

/*
 * @brief Event group to signal the status of the Ethernet connection.
 *
 * This event group is used to communicate the current status of the Ethernet connection
 * between different tasks. It provides flags that other tasks can check to determine
 * whether the device is connected to the network, has received an IP address, or is
 * facing disconnection issues. The event group helps in synchronizing Wi-Fi related
 * events across tasks in the system.
 */
EventGroupHandle_t wifi_event_group = {0};

/*
 * @brief Initialize the Non-Volatile Storage (NVS) for the device.
 *
 * This function initializes the NVS (Non-Volatile Storage) module, which is used to store
 * configuration data and other persistent information. If the NVS initialization fails due
 * to no free pages or a new version of NVS being found, it will erase the existing NVS data
 * and reinitialize it.
 *
 * @return 
 * - ESP_OK on successful initialization.
 * - Other error codes from `nvs_flash_init()` in case of failure.
 */
esp_err_t initialize_nvs(void) {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result;
}

void app_main() {

    ESP_ERROR_CHECK(initialize_nvs());
    wifi_event_group = xEventGroupCreate();
    xTaskCreate(
        network_execute,
        "Network Task",
        2048 * 10,
        NULL,
        tskIDLE_PRIORITY,
        NULL);

    vTaskDelay(pdMS_TO_TICKS(10000));
    network_set_credentials("NETPARQUE_PAOLA\0", "NPQ196253\0");
}