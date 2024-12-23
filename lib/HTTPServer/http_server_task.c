#include "esp_log.h"

#include "http_server_task.h"
#include "network_task.h"

static const char* TAG            = "HTTPServerProcess"; /**< Logging tag for HTTPServerProcess class. */
static httpd_config_t config      = HTTPD_DEFAULT_CONFIG();
static httpd_handle_t http_server = NULL;
static bool is_server_connected   = false;

/*
 * @brief Event Group to signal the status of the Wi-Fi connection.
 *
 * This event group is used to communicate the connection status of the ESP32's Wi-Fi interface
 * to other tasks in the system. Various bits in the event group will be set or cleared based on
 * Wi-Fi events (e.g., connected, disconnected, IP acquired) to allow tasks to react accordingly.
 */
extern EventGroupHandle_t wifi_event_group;

extern const uint8_t bin_data_index_html_start[] asm("_binary_index_html_start");          /**< Start of index.html binary data. */
extern const uint8_t bin_data_index_html_end[] asm("_binary_index_html_end");              /**< End of index.html binary data. */
extern const uint8_t bin_data_styles_css_start[] asm("_binary_styles_css_start");          /**< Start of styles.css binary data. */
extern const uint8_t bin_data_styles_css_end[] asm("_binary_styles_css_end");              /**< End of styles.css binary data. */
extern const uint8_t bin_data_app_js_start[] asm("_binary_app_js_start");                  /**< Start of app.js binary data. */
extern const uint8_t bin_data_app_js_end[] asm("_binary_app_js_end");                      /**< End of app.js binary data. */
extern const uint8_t bin_data_jquery3_js_start[] asm("_binary_jquery_3_3_1_min_js_start"); /**< Start of jquery-3.3.1.min.js binary data. */
extern const uint8_t bin_data_jquery3_js_end[] asm("_binary_jquery_3_3_1_min_js_end");     /**< End of jquery-3.3.1.min.js binary data. */
extern const uint8_t bin_data_favicon_ico_start[] asm("_binary_favicon_ico_start");        /**< Start of favicon.ico binary data. */
extern const uint8_t bin_data_favicon_ico_end[] asm("_binary_favicon_ico_end");            /**< End of favicon.ico binary data. */

/**
 * @brief HTTP GET handler for serving index.html.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t get_uri_index_html(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req,
                    (const char*)(bin_data_index_html_start),
                    bin_data_index_html_end - bin_data_index_html_start);

    return ESP_OK;
}

/**
 * @brief HTTP GET handler for serving styles.css.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t get_uri_get_app_css(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req,
                    (const char*)(bin_data_styles_css_start),
                    bin_data_styles_css_end - bin_data_styles_css_start);

    return ESP_OK;
}

/**
 * @brief HTTP GET handler for serving app.js.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t get_uri_get_app_js(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(req,
                    (const char*)(bin_data_app_js_start),
                    bin_data_app_js_end - bin_data_app_js_start - 1);

    return ESP_OK;
}

/**
 * @brief HTTP GET handler for serving jquery-3.3.1.min.js.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t get_uri_get_jquery_js(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(
        req, (const char*)(bin_data_jquery3_js_start),
        bin_data_jquery3_js_end - bin_data_jquery3_js_start - 1);

    return ESP_OK;
}

/**
 * @brief HTTP GET handler for serving favicon.ico.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t get_uri_favicon_icon(httpd_req_t* req) {
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(
        req, (const char*)(bin_data_favicon_ico_start),
        bin_data_favicon_ico_end - bin_data_favicon_ico_start);

    return ESP_OK;
}

/**
 * @brief HTTP POST handler for processing WiFi credentials.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t post_uri_wifi_credentials(httpd_req_t* req) {
    esp_err_t result  = ESP_OK;
    char ssid[32]     = {0};
    char password[64] = {0};

    uint8_t ssid_len = httpd_req_get_hdr_value_len(req, "my-connected-ssid") + 1;
    uint8_t pwd_len  = httpd_req_get_hdr_value_len(req, "my-connected-pwd") + 1;

    do {
        if (ssid_len <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "SSID size equals to 0!");
            return ESP_FAIL;
        }

        if (pwd_len <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "Password size equals to 0!");
            return ESP_FAIL;
        }

        if (httpd_req_get_hdr_value_str(req, "my-connected-ssid", ssid,
                                        ssid_len) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "Error reading SSID!");
            return ESP_FAIL;
        }

        if (httpd_req_get_hdr_value_str(req, "my-connected-pwd", password,
                                        pwd_len) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "Error reading Password!");
            return ESP_FAIL;
        }
        ssid[ssid_len] = '\0';
        password[pwd_len] = '\0';
        
        result = network_set_credentials(ssid, password);

    } while (0);

    return result;
}

/**
 * @brief Initializes the list of HTTP request URIs and their corresponding handlers.
 */
esp_err_t initialize_request_list(void) {
    static const httpd_uri_t uri_index_html = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = get_uri_index_html,
        .user_ctx = NULL,
    };

    static const httpd_uri_t uri_get_styles_css = {
        .uri      = "/styles.css",
        .method   = HTTP_GET,
        .handler  = get_uri_get_app_css,
        .user_ctx = NULL,
    };

    static const httpd_uri_t uri_get_app_js = {
        .uri      = "/app.js",
        .method   = HTTP_GET,
        .handler  = get_uri_get_app_js,
        .user_ctx = NULL,
    };

    static const httpd_uri_t uri_get_jquery_js = {
        .uri      = "/jquery-3.3.1.min.js",
        .method   = HTTP_GET,
        .handler  = get_uri_get_jquery_js,
        .user_ctx = NULL,
    };

    static const httpd_uri_t uri_get_favicon_ico = {
        .uri      = "/favicon.ico",
        .method   = HTTP_GET,
        .handler  = get_uri_favicon_icon,
        .user_ctx = NULL,
    };

    static const httpd_uri_t uri_post_credentials = {
        .uri      = "/wifiCredentials.json",
        .method   = HTTP_POST,
        .handler  = post_uri_wifi_credentials,
        .user_ctx = NULL,
    };

    esp_err_t result = ESP_OK;
    result += httpd_register_uri_handler(http_server, &uri_index_html);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += httpd_register_uri_handler(http_server, &uri_get_styles_css);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += httpd_register_uri_handler(http_server, &uri_get_app_js);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += httpd_register_uri_handler(http_server, &uri_get_jquery_js);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += httpd_register_uri_handler(http_server, &uri_get_favicon_ico);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += httpd_register_uri_handler(http_server, &uri_post_credentials);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    return result;
}

/**
 * @brief Starts the HTTP server and registers URI handlers.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    esp_err_t result      = httpd_start(&http_server, &config);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server started successfully");
        initialize_request_list();
        is_server_connected = true;
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(result));
    }
    return result;
}

/**
 * @brief Stops the HTTP server.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
void stop_http_server(void) {
    if (http_server) {
        httpd_stop(http_server);
        http_server         = NULL;
        is_server_connected = false;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}

/**
 * @brief Initializes the HTTPManager by configuring server parameters and initializing request handlers.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t http_server_task_initialize(void) {
    esp_err_t result = ESP_OK;

    config.send_wait_timeout = 10;
    config.recv_wait_timeout = 10;
    config.max_uri_handlers  = 20;

    return result;
}

/**
 * @brief Main execution function for the HTTP server.
 *
 * This function initializes and starts the HTTP server, enabling the ESP32 to
 * handle incoming web requests. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void http_server_task_execute(void* pvParameters) {
    if (http_server_task_initialize() != ESP_OK) {
        vTaskDelete(NULL);
    }

    while (1) {
        EventBits_t wifi_bits = xEventGroupWaitBits(wifi_event_group,
                                                    WIFI_CONNECTED_AP,
                                                    pdFALSE,
                                                    pdFALSE,
                                                    pdMS_TO_TICKS(100));

        if (is_server_connected) {
            if ((wifi_bits & WIFI_CONNECTED_AP) == 0) {
                stop_http_server();
            }
        } else {
            if (wifi_bits & WIFI_CONNECTED_AP) {
                start_http_server();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
