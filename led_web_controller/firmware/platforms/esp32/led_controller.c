#include "led_controller.h"
#include "app_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include <string.h>
#include <stdio.h>
static const char *TAG="LED";
static led_strip_handle_t s_strip;
static SemaphoreHandle_t s_mutex;
static led_state_t s_state={.enabled=false,.color="OFF"};
static void name_for(uint8_t r,uint8_t g,uint8_t b,char *out,size_t n){const char *x="CUSTOM";if(!r&&!g&&!b)x="OFF";else if(r==255&&!g&&!b)x="RED";else if(!r&&g==255&&!b)x="GREEN";else if(!r&&!g&&b==255)x="BLUE";else if(r==255&&g==255&&!b)x="YELLOW";else if(!r&&g==255&&b==255)x="CYAN";else if(r==255&&!g&&b==255)x="MAGENTA";else if(r==255&&g==255&&b==255)x="WHITE";snprintf(out,n,"%s",x);}
static esp_err_t apply_locked(void){esp_err_t e=led_strip_set_pixel(s_strip,0,s_state.red,s_state.green,s_state.blue);if(e!=ESP_OK)return e;return led_strip_refresh(s_strip);}
esp_err_t led_controller_init(void){s_mutex=xSemaphoreCreateMutex();if(!s_mutex)return ESP_ERR_NO_MEM;led_strip_config_t c={.strip_gpio_num=RGB_LED_GPIO,.max_leds=RGB_LED_COUNT};led_strip_rmt_config_t r={.resolution_hz=RGB_LED_RMT_RESOLUTION_HZ,.flags.with_dma=false};esp_err_t e=led_strip_new_rmt_device(&c,&r,&s_strip);if(e!=ESP_OK)return e;return led_strip_clear(s_strip);}
esp_err_t led_set_rgb(uint8_t r,uint8_t g,uint8_t b){if(xSemaphoreTake(s_mutex,pdMS_TO_TICKS(APP_LED_MUTEX_TIMEOUT_MS))!=pdTRUE)return ESP_ERR_TIMEOUT;s_state.red=r;s_state.green=g;s_state.blue=b;s_state.enabled=true;name_for(r,g,b,s_state.color,sizeof(s_state.color));esp_err_t e=apply_locked();if(e==ESP_OK)ESP_LOGI(TAG,"Color changed: %s (%u,%u,%u)",s_state.color,r,g,b);xSemaphoreGive(s_mutex);return e;}
esp_err_t led_set_color(const char *color){if(!color)return ESP_ERR_INVALID_ARG;uint8_t r=0,g=0,b=0;if(!strcasecmp(color,"RED"))r=255;else if(!strcasecmp(color,"GREEN"))g=255;else if(!strcasecmp(color,"BLUE"))b=255;else if(!strcasecmp(color,"YELLOW"))r=g=255;else if(!strcasecmp(color,"CYAN"))g=b=255;else if(!strcasecmp(color,"MAGENTA"))r=b=255;else if(!strcasecmp(color,"WHITE"))r=g=b=255;else if(!strcasecmp(color,"OFF"))return led_off();else return ESP_ERR_INVALID_ARG;return led_set_rgb(r,g,b);}
esp_err_t led_on(void){if(xSemaphoreTake(s_mutex,pdMS_TO_TICKS(APP_LED_MUTEX_TIMEOUT_MS))!=pdTRUE)return ESP_ERR_TIMEOUT;s_state.enabled=true;esp_err_t e=apply_locked();xSemaphoreGive(s_mutex);return e;}
esp_err_t led_off(void){if(xSemaphoreTake(s_mutex,pdMS_TO_TICKS(APP_LED_MUTEX_TIMEOUT_MS))!=pdTRUE)return ESP_ERR_TIMEOUT;s_state.enabled=false;esp_err_t e=led_strip_clear(s_strip);if(e==ESP_OK)ESP_LOGI(TAG,"LED turned OFF");xSemaphoreGive(s_mutex);return e;}
bool led_is_on(void){bool v=false;if(xSemaphoreTake(s_mutex,pdMS_TO_TICKS(APP_LED_MUTEX_TIMEOUT_MS))==pdTRUE){v=s_state.enabled;xSemaphoreGive(s_mutex);}return v;}
esp_err_t led_get_state(led_state_t *out){if(!out)return ESP_ERR_INVALID_ARG;if(xSemaphoreTake(s_mutex,pdMS_TO_TICKS(APP_LED_MUTEX_TIMEOUT_MS))!=pdTRUE)return ESP_ERR_TIMEOUT;*out=s_state;xSemaphoreGive(s_mutex);return ESP_OK;}
