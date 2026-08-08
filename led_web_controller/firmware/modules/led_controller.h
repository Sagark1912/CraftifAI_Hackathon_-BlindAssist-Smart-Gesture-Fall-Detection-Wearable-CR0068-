#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
typedef struct { bool enabled; uint8_t red; uint8_t green; uint8_t blue; char color[16]; } led_state_t;
esp_err_t led_controller_init(void);
esp_err_t led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);
esp_err_t led_set_color(const char *color);
esp_err_t led_on(void);
esp_err_t led_off(void);
bool led_is_on(void);
esp_err_t led_get_state(led_state_t *state);
#endif
