# Firmware Topology

## Physical topology

```text
┌──────────────────────┐       I2C        ┌──────────────────────────────┐
│      MPU6050         │─────────────────>│          ESP32-C3             │
│                      │ SDA → GPIO5      │                              │
│ VCC → 3.3 V          │ SCL → GPIO4      │ 100 Hz sampling               │
│ GND → GND            │                  │ Calibration                   │
│ AD0 → address select │                  │ Filtering                     │
└──────────────────────┘                  │ Gesture recognition           │
                                          │ Activity recognition          │
                                          │ Fall-response state machine   │
                                          │ Local Wi-Fi HTTP server       │
                                          └───────────────┬──────────────┘
                                                          │ Wi-Fi LAN
                                                          v
                                          ┌──────────────────────────────┐
                                          │ Smartphone browser            │
                                          │                              │
                                          │ HTML dashboard                │
                                          │ JSON polling                  │
                                          │ Web Speech API                │
                                          │ Phone speaker                 │
                                          └──────────────────────────────┘
```

Only the ESP32 and MPU6050 are physical project hardware. The smartphone is an external communication and speech-output device.

## Firmware topology

```text
main/entry.c
    │ app_main()
    v
firmware/app/app.c
    │
    ├── imu_if.h
    │      └── platforms/esp32/imu_mpu6050.c
    │              └── mpu6050.c
    │                    └── ESP-IDF I2C master
    │
    ├── services/recognizer.c
    │      ├── gesture state machine
    │      ├── activity classifier
    │      ├── fall detector
    │      └── action codes 0–8
    │
    ├── services/web_state.c
    │      ├── mutex-protected snapshot
    │      └── bounded event history
    │
    └── platforms/esp32/web_server.c
           ├── ESP-IDF Wi-Fi station
           ├── HTTP GET /
           ├── HTTP GET /api/state
           ├── HTTP GET /api/history
           └── embedded HTML + browser speech
```

## Runtime data flow

```text
MPU6050 sample at 100 Hz
        ↓
Calibration and filtering
        ↓
imu_sample_t
        ↓
recognizer_update()
        ↓
recognition_result_t
        ├── web_state_update() every valid sample
        └── web_state_record_event() only for new actions/events
                    ↓
             JSON snapshot/history
                    ↓
             phone browser polling
                    ↓
             one speech announcement per event ID
```

## State and event behavior

```text
MONITORING
   ├── LEFT          → USER_OK when responding to a fall
   ├── RIGHT         → NEEDS_HELP
   ├── DOUBLE_SHAKE  → EMERGENCY
   ├── FORWARD       → SELECT/CONFIRM
   ├── BACKWARD      → CANCEL/BACK
   └── fall sequence → WAITING_FOR_RESPONSE
                           ├── LEFT → alert cancelled
                           ├── RIGHT → help remains active
                           └── timeout → NO_RESPONSE emergency
```

## Action-code interface

| Code | Symbol | Meaning |
|---:|---|---|
| 0 | `ACTION_NONE` | No new action |
| 1 | `ACTION_LEFT_OK` | Left / user OK |
| 2 | `ACTION_RIGHT_HELP` | Right / user needs help |
| 3 | `ACTION_DOUBLE_SHAKE_EMERGENCY` | Emergency gesture |
| 4 | `ACTION_FORWARD_CONFIRM` | Forward / confirm |
| 5 | `ACTION_BACKWARD_CANCEL` | Backward / cancel |
| 6 | `ACTION_SINGLE_SHAKE_REPEAT` | Repeat/activate |
| 7 | `ACTION_FALL_DETECTED` | Fall confirmed |
| 8 | `ACTION_NO_RESPONSE` | Response timeout |

Action codes are local state values. They are not transmitted to Blynk or any cloud platform.

## Network and speech boundary

The ESP32 serves the page and JSON locally over Wi-Fi. It does not synthesize speech. The browser converts selected new event text into speech using `window.speechSynthesis`. The user must press **Enable Spoken Alerts**. Speech may stop if the browser is backgrounded, the phone is muted, or the operating system suspends the page.

The local HTTP server has no authentication or TLS and must remain on a trusted private network.
