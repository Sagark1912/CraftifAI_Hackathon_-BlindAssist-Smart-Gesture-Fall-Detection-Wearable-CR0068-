#include "web_state.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define WEB_HISTORY_CAPACITY 12
#define WEB_TEXT_SIZE 48

typedef struct {
    imu_sample_t sample;
    activity_t activity;
    system_state_t system_state;
    action_code_t action_code;
    float fall_score;
    bool fall_confirmed;
    bool wifi_connected;
    char ip[16];
    char gesture[WEB_TEXT_SIZE];
    char action[WEB_TEXT_SIZE];
    char event_code[WEB_TEXT_SIZE];
    uint32_t revision;
} web_snapshot_t;

typedef struct { uint32_t id; char text[WEB_TEXT_SIZE]; } history_item_t;
static web_snapshot_t snapshot;
static history_item_t history[WEB_HISTORY_CAPACITY];
static size_t history_count;
static uint32_t next_event_id;
static SemaphoreHandle_t lock;

void web_state_init(void)
{
    lock = xSemaphoreCreateMutex();
    memset(&snapshot, 0, sizeof(snapshot));
    strcpy(snapshot.gesture, "NONE");
    strcpy(snapshot.action, "NONE");
    strcpy(snapshot.event_code, "NONE");
    strcpy(snapshot.ip, "0.0.0.0");
}

void web_state_update(const imu_sample_t *sample, const recognition_result_t *result, bool wifi_connected, const char *ip)
{
    if (!lock || !sample || !result) return;
    xSemaphoreTake(lock, portMAX_DELAY);
    snapshot.sample = *sample;
    snapshot.activity = result->activity;
    snapshot.system_state = result->system_state;
    snapshot.action_code = result->action_code;
    snapshot.fall_score = result->fall_score;
    snapshot.fall_confirmed = result->fall_confirmed;
    snapshot.wifi_connected = wifi_connected;
    if (ip) snprintf(snapshot.ip, sizeof(snapshot.ip), "%s", ip);
    snprintf(snapshot.gesture, sizeof(snapshot.gesture), "%s", result->gesture);
    snprintf(snapshot.action, sizeof(snapshot.action), "%s", result->action);
    snprintf(snapshot.event_code, sizeof(snapshot.event_code), "%s", result->event_code);
    snapshot.revision++;
    xSemaphoreGive(lock);
}

void web_state_record_event(const recognition_result_t *result)
{
    if (!lock || !result || !result->response_event) return;
    xSemaphoreTake(lock, portMAX_DELAY);
    if (history_count < WEB_HISTORY_CAPACITY) history_count++;
    else memmove(&history[0], &history[1], sizeof(history[0]) * (WEB_HISTORY_CAPACITY - 1));
    history[history_count - 1].id = ++next_event_id;
    snprintf(history[history_count - 1].text, WEB_TEXT_SIZE, "%s: %s", result->event_code, result->action);
    xSemaphoreGive(lock);
}

int web_state_json(char *buffer, size_t size)
{
    if (!lock || !buffer || size == 0) return -1;
    xSemaphoreTake(lock, portMAX_DELAY);
    int n = snprintf(buffer, size,
        "{\"revision\":%lu,\"activity\":\"%s\",\"gesture\":\"%s\",\"action\":\"%s\",\"action_code\":%d,\"fall\":%s,\"fall_score\":%.2f,\"system\":\"%s\",\"event\":\"%s\",\"wifi\":%s,\"ip\":\"%s\",\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,\"tilt\":%.1f}",
        (unsigned long)snapshot.revision, activity_name(snapshot.activity), snapshot.gesture, snapshot.action, (int)snapshot.action_code,
        snapshot.fall_confirmed ? "true" : "false", snapshot.fall_score, system_state_name(snapshot.system_state), snapshot.event_code,
        snapshot.wifi_connected ? "true" : "false", snapshot.ip, snapshot.sample.ax_g, snapshot.sample.ay_g, snapshot.sample.az_g,
        snapshot.sample.gx_dps, snapshot.sample.gy_dps, snapshot.sample.gz_dps, snapshot.sample.roll_deg);
    xSemaphoreGive(lock);
    return n;
}

int web_history_json(char *buffer, size_t size)
{
    if (!lock || !buffer || size == 0) return -1;
    xSemaphoreTake(lock, portMAX_DELAY);
    int used = snprintf(buffer, size, "[");
    for (size_t i = 0; i < history_count && used > 0 && (size_t)used < size; ++i)
        used += snprintf(buffer + used, size - (size_t)used, "%s{\"id\":%lu,\"text\":\"%s\"}", i ? "," : "", (unsigned long)history[i].id, history[i].text);
    if (used > 0 && (size_t)used < size) used += snprintf(buffer + used, size - (size_t)used, "]");
    xSemaphoreGive(lock);
    return used;
}
