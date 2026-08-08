# Smart Assistive Wearable

This ESP32-C3 firmware uses one MPU6050 as its only physical sensor. It samples motion over I2C, calibrates the stationary device, estimates activity and orientation, recognizes intentional gestures, and evaluates multi-condition fall candidates. Blynk IoT provides Wi-Fi telemetry, event notifications, dashboard status, and phone accessibility feedback.

## Hardware

- MPU6050 VCC → ESP32-C3 3.3 V
- MPU6050 GND → ESP32-C3 GND
- MPU6050 SDA → ESP32-C3 GPIO5
- MPU6050 SCL → ESP32-C3 GPIO4
- Default MPU6050 address: 0x68; AD0 high selects 0x69

The smartphone and Blynk Cloud are communication and user-interface layers only. Blynk V14 carries an integer action code: 0 none, 1 left/OK, 2 right/help, 3 double-shake/emergency, 4 forward/confirm, 5 backward/cancel, 6 single-shake/repeat, 7 fall, and 8 no response. This prototype is not medically certified or safety-critical.

## Firmware modules

`firmware/platforms/esp32/mpu6050.c` implements ESP-IDF I2C access, address probing, calibration, filtering, and feature extraction. `firmware/services/recognizer.c` implements gesture, activity, fall, and response state machines. `firmware/platforms/esp32/blynk_client.c` implements Wi-Fi and Blynk HTTP telemetry/events. `firmware/app/app.c` orchestrates startup and the fixed-rate processing loop.

Build with ESP-IDF 5.5 for ESP32-C3. Configure local Wi-Fi and Blynk credentials in `firmware/configs/app_config.h`, keep the device still during the five-second startup calibration, and validate all thresholds with supervised, controlled tests before considering any field use.
