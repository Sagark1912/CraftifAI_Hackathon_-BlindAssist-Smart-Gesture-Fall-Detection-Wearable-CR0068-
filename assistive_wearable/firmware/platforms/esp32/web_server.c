#include "web_server.h"
#include "app_config.h"
#include "logger.h"
#include "web_state.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"

static const char *TAG = "web";
static bool connected;
static char ip_text[16] = "0.0.0.0";
static httpd_handle_t server;

static const char INDEX_HTML[] =
"<!doctype html><html lang='en'><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>Assistive Wearable</title>"
"<style>body{font-family:system-ui;background:#101820;color:#fff;margin:0;padding:18px}main{max-width:720px;margin:auto}.card{background:#1e2c38;padding:16px;margin:10px 0;border-radius:12px}h1{font-size:1.5rem}.value{font-size:1.2rem;font-weight:700;color:#7ee787}button{padding:12px;font-size:1rem;border-radius:8px}li{margin:8px 0}</style></head>"
"<body><main><h1>Smart Assistive Wearable</h1><button onclick='enableSpeech()'>Enable spoken alerts</button><div class='card' id='state'>Connecting...</div><div class='card'><h2>Event history</h2><ul id='events'></ul></div></main>"
"<script>let spoken=false,last=0;function enableSpeech(){spoken=true;speechSynthesis.speak(new SpeechSynthesisUtterance('Spoken alerts enabled.'));}function say(t){if(spoken)speechSynthesis.speak(new SpeechSynthesisUtterance(t));}async function poll(){try{let s=await (await fetch('/api/state')).json();document.getElementById('state').innerHTML=`<div class=value>Activity: ${s.activity}</div><p>Gesture: ${s.gesture}</p><p>Action: ${s.action} (code ${s.action_code})</p><p>Fall: ${s.fall?'CONFIRMED':'Normal'}</p><p>System: ${s.system}</p><p>Wi-Fi: ${s.wifi?'Connected':'Offline'} ${s.ip}</p><p>Accel: ${s.ax}, ${s.ay}, ${s.az}</p><p>Gyro: ${s.gx}, ${s.gy}, ${s.gz}</p><p>Tilt: ${s.tilt} degrees</p>`;let h=await (await fetch('/api/history')).json();document.getElementById('events').innerHTML=h.map(e=>`<li>${e.id}: ${e.text}</li>`).join('');for(let e of h)if(e.id>last){if(e.text.includes('FALL'))say('Possible fall detected. Are you okay?');else if(e.text.includes('USER_OK'))say('Emergency alert cancelled.');else if(e.text.includes('HELP')||e.text.includes('EMERGENCY'))say('Emergency help requested.');else if(e.text.includes('NO_RESPONSE'))say('No response. Emergency state activated.');last=Math.max(last,e.id);}}catch(e){document.getElementById('state').textContent='Phone cannot reach ESP32';}}setInterval(poll,500);poll();</script></body></html>";

static esp_err_t send(httpd_req_t *req, const char *type, const char *body)
{
    httpd_resp_set_type(req, type);
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}
static esp_err_t root_handler(httpd_req_t *req) { return send(req, "text/html", INDEX_HTML); }
static esp_err_t state_handler(httpd_req_t *req) { char body[1024]; web_state_json(body, sizeof(body)); return send(req, "application/json", body); }
static esp_err_t history_handler(httpd_req_t *req) { char body[1024]; web_history_json(body, sizeof(body)); return send(req, "application/json", body); }

static void wifi_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) { connected = false; esp_wifi_connect(); }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = data;
        snprintf(ip_text, sizeof(ip_text), IPSTR, IP2STR(&event->ip_info.ip));
        connected = true;
        ESP_LOGI(TAG, "Web page available at http://%s/", ip_text);
    }
}

void web_server_init(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(err);
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(err);
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_handler, NULL));
    wifi_config_t wifi = { .sta = { .ssid = APP_WIFI_SSID, .password = APP_WIFI_PASSWORD, .threshold.authmode = WIFI_AUTH_WPA2_PSK } };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
    ESP_ERROR_CHECK(esp_wifi_start());
    httpd_config_t server_cfg = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &server_cfg));
    httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_handler};
    httpd_uri_t state = {.uri = "/api/state", .method = HTTP_GET, .handler = state_handler};
    httpd_uri_t history = {.uri = "/api/history", .method = HTTP_GET, .handler = history_handler};
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &state));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &history));
}
bool web_server_is_connected(void) { return connected; }
const char *web_server_ip(void) { return ip_text; }
