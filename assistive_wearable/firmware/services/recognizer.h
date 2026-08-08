#ifndef RECOGNIZER_H
#define RECOGNIZER_H
#include <stdbool.h>
#include <stdint.h>
#include "imu_if.h"

typedef enum { ACT_IDLE, ACT_STANDING, ACT_WALKING, ACT_RUNNING, ACT_SITTING, ACT_FALL_CANDIDATE, ACT_FALL_CONFIRMED } activity_t;
typedef enum { SYS_MONITORING, SYS_FALL_WAITING, SYS_USER_OK, SYS_NEEDS_HELP, SYS_EMERGENCY, SYS_NO_RESPONSE } system_state_t;

/* Stable integer action codes published to Blynk V14. */
typedef enum {
    ACTION_NONE = 0,
    ACTION_LEFT_OK = 1,
    ACTION_RIGHT_HELP = 2,
    ACTION_DOUBLE_SHAKE_EMERGENCY = 3,
    ACTION_FORWARD_CONFIRM = 4,
    ACTION_BACKWARD_CANCEL = 5,
    ACTION_SINGLE_SHAKE_REPEAT = 6,
    ACTION_FALL_DETECTED = 7,
    ACTION_NO_RESPONSE = 8
} action_code_t;

typedef struct {
    const char *gesture;
    const char *action;
    const char *event_code;
    activity_t activity;
    system_state_t system_state;
    action_code_t action_code;
    float fall_score;
    bool fall_confirmed;
    bool response_event;
} recognition_result_t;

void recognizer_init(void);
void recognizer_update(const imu_sample_t *sample, uint32_t now_ms, recognition_result_t *result);
const char *activity_name(activity_t activity);
const char *system_state_name(system_state_t state);
#endif
