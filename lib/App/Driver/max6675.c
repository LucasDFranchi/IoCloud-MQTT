#include "max6675.h"
#include "esp_log.h"
#include "driver/spi_master.h"

spi_device_handle_t max6675_handle;

static const char *TAG                       = "MAX6675";  ///< Tag used for logging.
static const uint8_t TRANSACTION_DATA_LENGTH = 16;         ///< Length of the SPI transaction data.

/**
 * @brief Initializes the MAX6675 SPI communication.
 *
 * Configures the SPI bus and attaches the MAX6675 sensor as an SPI device.
 * This function allows the user to specify the MISO, CLK, and CS pins dynamically.
 *
 * @param miso_pin GPIO number for the SPI MISO pin.
 * @param clk_pin GPIO number for the SPI clock (SCLK) pin.
 * @param cs_pin GPIO number for the SPI chip select (CS) pin.
 *
 * @return esp_err_t ESP_OK on success, or an error code if initialization fails.
 */
esp_err_t max6675_initialize(int miso_pin, int clk_pin, int cs_pin) {
    spi_bus_config_t buscfg = {
        .miso_io_num   = miso_pin,
        .mosi_io_num   = -1,  // MAX6675 is read-only, no MOSI
        .sclk_io_num   = clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,  // 1 MHz
        .mode           = 0,        // SPI mode 0
        .spics_io_num   = cs_pin,
        .queue_size     = 1,
    };

    esp_err_t result = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
    if (result != ESP_OK) {
        return result;
    }

    return spi_bus_add_device(SPI2_HOST, &devcfg, &max6675_handle);
}

/**
 * @brief Reads the temperature from the MAX6675 sensor and applies calibration.
 *
 * Performs an SPI transaction to retrieve the raw temperature data, checks  
 * for errors such as connection issues, and converts the value to Celsius.  
 * The function also applies the provided gain and offset calibration.
 *
 * @param[in] gain   Calibration gain factor to adjust the temperature reading.
 * @param[in] offset Calibration offset to fine-tune the final temperature value.
 *
 * @return float Temperature in Celsius (°C). Returns -1.0 on failure.
 */
float max6675_get_temperature(int gain, int offset) {
    uint8_t data[2]         = {0};
    spi_transaction_t trans = {
        .flags    = SPI_TRANS_USE_RXDATA,
        .length   = TRANSACTION_DATA_LENGTH,  // MAX6675 sends 16 bits
        .rxlength = TRANSACTION_DATA_LENGTH,
    };

    esp_err_t result = spi_device_transmit(max6675_handle, &trans);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed!");
        return result;
    }

    data[0] = trans.rx_data[0];
    data[1] = trans.rx_data[1];

    if (data[1] & 0x04) {
        ESP_LOGE(TAG, "Thermocouple not connected!");
        return ESP_FAIL;
    }
    int16_t raw_value = ((data[0] << 8) | data[1]) >> 3;
    float temperature = ((raw_value * 0.25) * gain) + offset;

    ESP_LOGI(TAG, "Temperature: %.2f°C", temperature);
    return temperature; 
}
