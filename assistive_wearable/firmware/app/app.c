#include "app.h"
#include "app_config.h"
#include "logger.h"
#include "imu_if.h"
#include "recognizer.h"
#include "web_server.h"
#include "web_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include <stdbool.h>

static const char *TAG = "app";

void app_start(void)
{
    esp_err_t nvs = nvs_flash_init();
    if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs);
    esp_err_t imu_status = imu_init();
    while (imu_status != ESP_OK) {
        ESP_LOGW(TAG, "MPU6050 unavailable: %s; retrying", esp_err_to_name(imu_status));
        vTaskDelay(pdMS_TO_TICKS(5000));
        imu_status = imu_init();
    }
    ESP_LOGI(TAG, "Keep the wearable still for %u seconds", APP_CALIBRATION_SECONDS);
    ESP_ERROR_CHECK(imu_calibrate(APP_CALIBRATION_SECONDS));
    recognizer_init();
    web_state_init();
    web_server_init();

    uint32_t next_sample = 0;
    while (true) {
        uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if (now >= next_sample) {
            imu_sample_t sample;
            if (imu_read(&sample) == ESP_OK) {
                recognition_result_t result;
                recognizer_update(&sample, now, &result);
                if (result.response_event) {
                    web_state_record_event(&result);
                    ESP_LOGI(TAG, "gesture=%s action=%s code=%d event=%s", result.gesture, result.action, (int)result.action_code, result.event_code);
                }
                web_state_update(&sample, &result, web_server_is_connected(), web_server_ip());
            } else {
                ESP_LOGW(TAG, "MPU6050 read failed");
            }
            next_sample = now + (1000 / APP_SAMPLE_HZ);
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
