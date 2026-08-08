#include "app.h"
#include "app_config.h"
#include "logger.h"
#include "led_controller.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

static void startup_sequence(void)
{
    ESP_LOGI("STARTUP", "Starting startup sequence");
    ESP_ERROR_CHECK(led_set_rgb(255, 0, 0));
    ESP_LOGI("STARTUP", "Blink 1");
    vTaskDelay(pdMS_TO_TICKS(STARTUP_BLINK_TIME_MS));
    ESP_ERROR_CHECK(led_off());
    vTaskDelay(pdMS_TO_TICKS(STARTUP_PAUSE_TIME_MS));
    ESP_ERROR_CHECK(led_set_rgb(0, 255, 0));
    ESP_LOGI("STARTUP", "Blink 2");
    vTaskDelay(pdMS_TO_TICKS(STARTUP_BLINK_TIME_MS));
    ESP_ERROR_CHECK(led_off());
    vTaskDelay(pdMS_TO_TICKS(STARTUP_PAUSE_TIME_MS));
    ESP_ERROR_CHECK(led_set_color("BLUE"));
    ESP_LOGI("STARTUP", "Startup colour changed to BLUE");
    vTaskDelay(pdMS_TO_TICKS(STARTUP_COLOR_HOLD_MS));
}

void app_start(void)
{
    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);

    ESP_LOGI(TAG, "ESP32-C3 RGB LED Controller");
    ESP_LOGI(TAG, "Native ESP-IDF firmware");
    ESP_LOGI("INIT", "RGB LED GPIO: %d", RGB_LED_GPIO);
    ESP_ERROR_CHECK(led_controller_init());
    ESP_LOGI("INIT", "RGB LED driver initialized");
    startup_sequence();

    ESP_ERROR_CHECK(wifi_manager_start());
    if (!wifi_manager_wait_for_connection(pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS))) {
        ESP_LOGW("WIFI", "Connection timeout; HTTP server will remain available if networking recovers");
    }
    if (web_server_start() != ESP_OK) {
        ESP_LOGE("WEB", "HTTP server failed to start");
        ESP_ERROR_CHECK(led_off());
        return;
    }
    ESP_LOGI("STATE", "System ready");
}
