#include <stdbool.h>

#include "app_config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#define TAG "blink"
void app_main(void)
{
    led_strip_handle_t led_strip = NULL;
    const led_strip_config_t strip_config = {
        .strip_gpio_num = APP_LED_GPIO,
        .max_leds = APP_LED_COUNT,
    };
    const led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
    ESP_LOGI(TAG, "RGB LED initialized on GPIO %d", APP_LED_GPIO);

    bool on = false;
    while (true) {
        on = !on;
        if (on) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, 32, 32, 32));
        } else {
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
        }
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        ESP_LOGI(TAG, "RGB LED %s", on ? "on" : "off");
        vTaskDelay(pdMS_TO_TICKS(APP_BLINK_PERIOD_MS));
    }
}
