# CraftifAI_Hackathon_-BlindAssist-Smart-Gesture-Fall-Detection-Wearable-CR0068-

# ESP32 + MPU6050 Smart Assistive Wearable

## Local Web Dashboard with Voice Feedback

Develop a complete working firmware for an **ESP32-C3 + MPU6050** assistive wearable designed for visually impaired users.

The system must perform **gesture recognition, activity recognition, and fall detection locally on the ESP32**, while providing a phone-based dashboard and spoken feedback through a local web interface.

---

## 1. Hardware Restriction

Use only:

* ESP32-C3
* MPU6050

The MPU6050 must be the **only physical sensor**.

Do NOT use:

* Blynk
* GPS
* GSM/4G
* Ultrasonic sensors
* ToF sensors
* Additional IMUs
* Cameras
* External sensors
* External display
* External buttons

The smartphone is only used as a **web browser, display, and audio/voice-feedback device**.

---

## 2. Remove Blynk Completely

Remove all Blynk-related functionality from the firmware.

Remove:

* Blynk library
* Blynk credentials
* Blynk authentication
* Blynk Cloud communication
* Blynk Events
* Blynk Datastreams
* `Blynk.virtualWrite()`
* Blynk timers
* `esp_http_client` if it was only being used for Blynk/cloud communication
* Any Blynk-specific configuration

There must be **no dependency on Blynk Cloud**.

---

## 3. Wi-Fi Operation

Keep ESP32 Wi-Fi in **Station (STA) mode**.

The ESP32 should connect to the user's Wi-Fi network.

After connecting, print the ESP32 IP address to Serial Monitor:

```text
WiFi connected
ESP32 IP: 192.168.x.x
```

The user should open the following address on the phone:

```text
http://<ESP32-IP-address>/
```

The phone and ESP32 must be connected to the same local Wi-Fi network.

---

## 4. Local Web Server

Host the entire dashboard directly from the ESP32.

The ESP32 should provide:

```text
GET /
GET /api/state
GET /api/history
GET /api/action
```

Additional endpoints can be added if useful.

The dashboard must be served directly from the ESP32 and must not require:

* Internet access
* Blynk Cloud
* External web hosting
* Firebase
* Any cloud server

---

# 5. Dashboard

Create an accessible, mobile-friendly HTML dashboard.

The dashboard should display:

### System Status

```text
ESP32 Status: ONLINE

Wi-Fi:
CONNECTED

MPU6050:
OK
```

### Current Activity

```text
Activity:
WALKING
```

Possible states:

* IDLE
* STANDING
* WALKING
* RUNNING
* UNKNOWN

### Last Gesture

```text
Last Gesture:
LEFT
```

### Fall Status

```text
Fall Status:
NORMAL
```

Possible states:

* NORMAL
* FALL SUSPECTED
* FALL CONFIRMED
* WAITING FOR RESPONSE
* USER OK
* USER NEEDS HELP
* EMERGENCY

### Current Action

Display the latest action code and its meaning.

---

# 6. Action Codes

Keep the existing action-code system.

```text
0 = None
1 = Left / User OK
2 = Right / User needs help
3 = Double shake / Emergency
4 = Forward / Confirm
5 = Backward / Cancel
6 = Single shake / Repeat
7 = Fall detected
8 = No response
```

The firmware should maintain the latest action code internally.

The browser should retrieve the current state through:

```text
/api/state
```

---

# 7. Gesture Recognition

Use only MPU6050 accelerometer and gyroscope data.

Implement:

### Left gesture

Meaning:

```text
USER OK
```

Spoken feedback:

> Left gesture detected. User okay.

### Right gesture

Meaning:

```text
USER NEEDS HELP
```

Spoken feedback:

> Right gesture detected. User needs help.

### Double shake

Meaning:

```text
EMERGENCY
```

Spoken feedback:

> Emergency gesture detected.

### Forward

Meaning:

```text
CONFIRM
```

### Backward

Meaning:

```text
CANCEL
```

### Single shake

Meaning:

```text
REPEAT
```

The gesture algorithm must use appropriate:

* acceleration thresholds
* gyroscope thresholds
* direction detection
* gesture duration
* cooldown/debounce
* state machine

to prevent accidental repeated gesture detection.

---

# 8. Fall Detection

Fall detection must remain completely local to the ESP32.

Do NOT send raw MPU6050 data to a cloud service for fall detection.

Use:

* acceleration magnitude
* gyroscope magnitude
* sudden impact
* orientation change
* post-impact inactivity
* timing information

A possible state machine is:

```text
NORMAL
   ↓
IMPACT SUSPECTED
   ↓
ORIENTATION CHANGE
   ↓
POST-IMPACT INACTIVITY
   ↓
FALL CONFIRMED
   ↓
WAIT FOR USER RESPONSE
   ↓
OK / NEED HELP / NO RESPONSE
```

Do not classify a fall based only on one acceleration threshold.

---

# 9. Fall Response

When a fall is confirmed:

```text
FALL DETECTED
      ↓
Action Code = 7
      ↓
Browser receives new event
      ↓
Phone speaks:
"Possible fall detected. Are you okay?"
      ↓
Start response timeout
```

The user then performs a gesture.

### Left

```text
Action Code = 1
```

Meaning:

```text
USER OK
```

Speak:

> Left gesture detected. User okay.

Cancel the emergency response state.

### Right

```text
Action Code = 2
```

Meaning:

```text
USER NEEDS HELP
```

Speak:

> Right gesture detected. User needs help.

Set the system to an emergency/help state.

### No response

If the configured timeout expires:

```text
Action Code = 8
```

Speak:

> No response received. Emergency state activated.

Set the system state to:

```text
EMERGENCY
```

---

# 10. Spoken Alerts

Use the **Web Speech API** available in modern mobile browsers.

The browser, not the ESP32, should generate the speech.

Example:

```javascript
window.speechSynthesis.speak(...)
```

The ESP32 should only provide event/state information.

---

# 11. Enable Spoken Alerts Button

Because mobile browsers may block automatic speech until the user interacts with the page, provide a prominent button:

```text
┌────────────────────────────┐
│                            │
│  🔊 ENABLE SPOKEN ALERTS   │
│                            │
└────────────────────────────┘
```

When the user presses it:

```text
Speech enabled
```

The browser should then be permitted to speak subsequent events.

Display:

```text
Spoken Alerts:
ENABLED
```

or:

```text
Spoken Alerts:
DISABLED
```

---

# 12. Do Not Repeat Spoken Events

The browser must speak **only new events**.

Do NOT repeatedly speak the same event every time the dashboard polls `/api/state`.

For example, if:

```text
Action Code = 7
```

the browser should speak:

> Possible fall detected. Are you okay?

only once.

It should not repeat that message every second.

Use an event ID or monotonically increasing event counter.

Example:

```json
{
  "eventId": 25,
  "actionCode": 7,
  "event": "FALL_DETECTED",
  "timestamp": 123456
}
```

The browser stores the last spoken `eventId`.

If the same event ID is received again:

```text
DO NOT SPEAK
```

If a new event ID appears:

```text
SPEAK EVENT
```

---

# 13. Event History

Maintain a limited event history on the ESP32.

Example:

```text
14:32:10  WALKING
14:35:22  LEFT_GESTURE
14:41:05  FALL_DETECTED
14:41:12  USER_OK
```

Use a circular buffer so memory usage remains controlled.

Provide:

```text
GET /api/history
```

Return JSON.

Example:

```json
{
  "events": [
    {
      "id": 21,
      "type": "LEFT_GESTURE",
      "actionCode": 1,
      "time": 12345
    },
    {
      "id": 22,
      "type": "FALL_DETECTED",
      "actionCode": 7,
      "time": 12360
    }
  ]
}
```

---

# 14. Current State API

Provide:

```text
GET /api/state
```

Example response:

```json
{
  "activity": "WALKING",
  "gesture": "LEFT",
  "fallStatus": "NORMAL",
  "systemState": "MONITORING",
  "actionCode": 1,
  "eventId": 25,
  "spokenAlerts": true
}
```

Do not expose unnecessary internal data.

---

# 15. Accessible Dashboard

The dashboard is intended for an assistive application.

Use:

* large text
* high contrast
* clear status indicators
* large buttons
* simple wording
* responsive mobile layout
* semantic HTML
* accessible labels
* ARIA attributes where useful

Do not depend exclusively on color to communicate status.

For example:

```text
Fall Status:
NORMAL
```

rather than only showing a green circle.

---

# 16. MPU6050 Data

The ESP32 should process:

### Accelerometer

```text
AX
AY
AZ
```

### Gyroscope

```text
GX
GY
GZ
```

Calculate useful derived values such as:

* acceleration magnitude
* gyro magnitude
* roll
* pitch
* movement state

Raw values can be shown on the dashboard for debugging.

---

# 17. Local Operation Requirement

The gesture, activity and fall algorithms must continue operating even if Wi-Fi becomes unavailable.

Architecture:

```text
                 MPU6050
                    ↓
                  ESP32
                    ↓
        ┌───────────┼───────────┐
        ↓           ↓           ↓
     Gesture      Activity     Fall
     Detection   Recognition  Detection
        │           │           │
        └───────────┼───────────┘
                    ↓
              Local Event State
                    ↓
               Web Server
                    ↓
               Phone Browser
```

If Wi-Fi disconnects:

```text
Gesture detection → CONTINUES
Fall detection → CONTINUES
Activity recognition → CONTINUES
Event logging → CONTINUES
Web dashboard → TEMPORARILY UNAVAILABLE
```

When Wi-Fi reconnects, the web dashboard should become accessible again and the stored recent history should still be available.

---

# 18. Avoid Blocking Code

Use non-blocking programming.

Avoid long:

```cpp
delay()
```

calls during sensor processing.

Use:

```cpp
millis()
```

for:

* sensor sampling
* gesture timeout
* fall response timeout
* event cooldown
* web polling/state updates

The web server must remain responsive while the MPU6050 is being processed.

---

# 19. Recommended Firmware Structure

Organize the code into logical modules/functions:

```cpp
setupWiFi()
setupMPU6050()
setupWebServer()

readMPU6050()

calculateMotionMetrics()

detectGesture()
detectActivity()
detectFall()

handleFallResponse()

createEvent()
addEventToHistory()

getCurrentStateJSON()
getHistoryJSON()

handleRoot()
handleStateAPI()
handleHistoryAPI()
```

Keep sensor processing separate from HTTP handling.

---

# 20. Dashboard Polling

The browser can periodically request:

```text
/api/state
```

For example, every 500–1000 ms.

However, the browser must not speak based simply on every polling cycle.

Use:

```text
eventId
```

to identify new events.

Only a new event should trigger speech.

---

# 21. Spoken Messages

Implement these exact default messages:

```text
Left gesture detected. User okay.

Right gesture detected. User needs help.

Emergency gesture detected.

Possible fall detected. Are you okay?

No response received. Emergency state activated.

Forward gesture detected. Confirm.

Backward gesture detected. Cancel.

Single shake detected. Repeating.

Walking detected.

Standing detected.

System ready.

Wi-Fi disconnected.

Wi-Fi reconnected.
```

Keep messages short and easy to understand.

---

# 22. Serial Monitor

Provide useful debugging output:

```text
[WiFi] Connecting...
[WiFi] Connected
[WiFi] IP: 192.168.1.25

[MPU6050] Initialized

[GESTURE] LEFT
[ACTION] 1

[FALL] Suspected
[FALL] Confirmed
[ACTION] 7

[EVENT] FALL_DETECTED
```

Avoid printing thousands of raw sensor values continuously unless debug mode is enabled.

---

# 23. Configuration

Create configurable constants for:

* MPU6050 sampling rate
* gesture thresholds
* gesture cooldown
* fall impact threshold
* orientation-change threshold
* inactivity duration
* fall response timeout
* history size
* web polling interval

Clearly document how to tune them.

---

# 24. Complete Deliverables

Provide:

1. Complete ESP32 Arduino firmware.
2. Complete HTML/CSS/JavaScript dashboard.
3. MPU6050 wiring.
4. Wi-Fi configuration instructions.
5. Phone connection instructions.
6. URL format:
   `http://<ESP32-IP-address>/`
7. Action-code table.
8. API documentation.
9. Gesture algorithm explanation.
10. Fall-detection algorithm explanation.
11. Fall-response state machine.
12. Web Speech API explanation.
13. Spoken-alert enable procedure.
14. Event-history implementation.
15. Testing procedure.
16. Troubleshooting guide.
17. Limitations and safety considerations.

---

# 25. Important Safety Limitation

This is an engineering prototype and must not be presented as a medically certified or life-safety device.

An MPU6050-only fall detector can produce false positives and false negatives. Extensive controlled and real-world testing would be required before considering any safety-critical deployment.

The system should be described as an **assistive technology prototype**.

---

## Final Architecture

```text
             ┌───────────────┐
             │    MPU6050    │
             │ Accelerometer │
             │  Gyroscope    │
             └───────┬───────┘
                     │ I²C
                     ↓
             ┌───────────────┐
             │    ESP32-C3   │
             │               │
             │ Gesture       │
             │ Activity      │
             │ Fall          │
             │ Detection     │
             └───────┬───────┘
                     │
                  Wi-Fi STA
                     │
                     ↓
             ┌───────────────┐
             │ Local Web     │
             │ Server        │
             └───────┬───────┘
                     │
             http://ESP32-IP/
                     │
                     ↓
             ┌───────────────┐
             │ Phone Browser │
             │               │
             │ Dashboard     │
             │ Notifications │
             │ Web Speech    │
             └───────────────┘

Physical hardware:
ESP32-C3 + MPU6050 ONLY
```

The final system must be completely independent of Blynk and any cloud service while still providing a useful **phone-based dashboard and spoken feedback through the local ESP32 web server**.
