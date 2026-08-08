#ifndef WEB_STATE_H
#define WEB_STATE_H
#include <stdbool.h>
#include <stdint.h>
#include "imu_if.h"
#include "recognizer.h"

void web_state_init(void);
void web_state_update(const imu_sample_t *sample, const recognition_result_t *result, bool wifi_connected, const char *ip);
void web_state_record_event(const recognition_result_t *result);
int web_state_json(char *buffer, size_t size);
int web_history_json(char *buffer, size_t size);
#endif
