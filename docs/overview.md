# ESP32-C3 RGB LED Blink

## Purpose

This firmware blinks the onboard addressable RGB LED on an ESP32-C3 DevKitM-1. The LED is driven through Espressif's `led_strip` RMT component and alternates between dim white and off every 500 ms.

## Hardware and configuration

- Target: ESP32-C3
- RGB LED data: GPIO8
- Pixels: 1
- On duration: 500 ms
- Off duration: 500 ms

Application constants are defined in `main/app_config.h`.

## Build and flash

Configure the ESP-IDF environment, then build and flash with the standard ESP-IDF project commands. The serial monitor logs initialization and each LED state transition at 115200 baud.

## Modules

- `main/main.c`: application entry point, LED-strip initialization, and FreeRTOS blink loop.
- `main/app_config.h`: application-level GPIO and timing configuration.
- `main/idf_component.yml`: managed dependency declaration for `espressif/led_strip`.
