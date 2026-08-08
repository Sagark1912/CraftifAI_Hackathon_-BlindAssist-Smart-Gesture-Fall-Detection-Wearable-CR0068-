#include "recognizer.h"
#include "app_config.h"
#include <math.h>
#include <string.h>

typedef struct {
    float last_accel;
    float max_dynamic;
    uint32_t last_shake;
    uint32_t last_gesture;
    uint32_t candidate_at;
    uint32_t active_motion;
    uint32_t gesture_started;
    float energy;
    bool candidate;
    bool confirmed;
    bool response_reported;
    bool gesture_active;
    bool shake_high;
} state_t;
static state_t s;

void recognizer_init(void) { memset(&s, 0, sizeof(s)); }

const char *activity_name(activity_t a)
{
    static const char *names[] = {"Idle", "Standing", "Walking", "Running", "Sitting", "Fall candidate", "Fall confirmed"};
    return a <= ACT_FALL_CONFIRMED ? names[a] : "Unknown";
}

const char *system_state_name(system_state_t state)
{
    static const char *names[] = {"MONITORING", "WAITING_FOR_RESPONSE", "USER_OK", "NEEDS_HELP", "EMERGENCY", "NO_RESPONSE"};
    return state <= SYS_NO_RESPONSE ? names[state] : "UNKNOWN";
}

static void set_event(recognition_result_t *r, const char *gesture, const char *action, const char *event)
{
    r->gesture = gesture;
    r->action = action;
    r->event_code = event;
    r->response_event = true;
}

void recognizer_update(const imu_sample_t *x, uint32_t now, recognition_result_t *r)
{
    memset(r, 0, sizeof(*r));
    r->gesture = "NONE";
    r->action = "NONE";
    r->event_code = "NONE";
    r->activity = ACT_IDLE;
    r->system_state = SYS_MONITORING;
    r->action_code = ACTION_NONE;

    float jerk = fabsf(x->accel_mag_g - s.last_accel);
    s.last_accel = x->accel_mag_g;
    s.energy = 0.95f * s.energy + 0.05f * (x->dynamic_accel_g + x->gyro_mag_dps / 180.0f);

    if (!s.candidate && !s.confirmed &&
        x->dynamic_accel_g > APP_FALL_IMPACT_G &&
        x->gyro_mag_dps > APP_FALL_GYRO_DPS && jerk > APP_FALL_JERK_G) {
        s.candidate = true;
        s.candidate_at = now;
        s.active_motion = now;
        s.max_dynamic = x->dynamic_accel_g;
    }

    if (s.candidate || s.confirmed) {
        if (x->gyro_mag_dps > 35.0f || x->dynamic_accel_g > 0.25f) s.active_motion = now;
        if (x->dynamic_accel_g > s.max_dynamic) s.max_dynamic = x->dynamic_accel_g;
        bool posture_change = fabsf(x->roll_deg) > APP_FALL_POSTURE_DEG || fabsf(x->pitch_deg) > APP_FALL_POSTURE_DEG;
        bool inactive = now - s.active_motion > APP_FALL_INACTIVITY_MS;
        if (s.candidate && posture_change && inactive && now - s.candidate_at >= APP_FALL_CONFIRM_MS) s.confirmed = true;
        if (s.candidate && !s.confirmed && now - s.candidate_at > APP_FALL_RESPONSE_TIMEOUT_MS) s.candidate = false;
    }

    if (s.confirmed && !s.response_reported) {
        r->activity = ACT_FALL_CONFIRMED;
        r->system_state = SYS_FALL_WAITING;
        r->fall_confirmed = true;
        r->fall_score = 1.0f;
        set_event(r, "FALL_CONFIRMED", "ARE YOU OK?", "FALL_DETECTED");
        r->action_code = ACTION_FALL_DETECTED;
        s.response_reported = true;
        return;
    }

    if (s.confirmed) {
        r->activity = ACT_FALL_CONFIRMED;
        r->system_state = SYS_FALL_WAITING;
        r->fall_confirmed = true;
        r->fall_score = 1.0f;
        if (now - s.candidate_at >= APP_FALL_CONFIRM_MS + APP_FALL_RESPONSE_TIMEOUT_MS) {
            r->system_state = SYS_NO_RESPONSE;
            set_event(r, "NO_RESPONSE", "EMERGENCY", "NO_RESPONSE");
            r->action_code = ACTION_NO_RESPONSE;
            s.confirmed = false;
            s.candidate = false;
            s.response_reported = false;
        }
        return;
    }

    if (s.candidate) {
        r->activity = ACT_FALL_CANDIDATE;
        r->system_state = SYS_FALL_WAITING;
        r->fall_score = 0.6f;
        return;
    }

    if (s.energy < 0.035f) r->activity = ACT_IDLE;
    else if (s.energy > 0.65f) r->activity = ACT_RUNNING;
    else if (s.energy > 0.16f) r->activity = ACT_WALKING;
    else if (fabsf(x->pitch_deg) > 45.0f) r->activity = ACT_SITTING;
    else r->activity = ACT_STANDING;

    bool cooldown = now - s.last_gesture >= APP_GESTURE_COOLDOWN_MS;
    bool shake = x->gyro_mag_dps > APP_SHAKE_GYRO_DPS && x->dynamic_accel_g > APP_SHAKE_DYNAMIC_G;
    if (shake && !s.shake_high && cooldown) {
        s.shake_high = true;
        s.last_gesture = now;
        if (s.last_shake != 0 && now - s.last_shake <= APP_SHAKE_PAIR_WINDOW_MS) {
            set_event(r, "DOUBLE_SHAKE", "EMERGENCY", "EMERGENCY_GESTURE");
            r->action_code = ACTION_DOUBLE_SHAKE_EMERGENCY;
        } else {
            set_event(r, "SHAKE", "REPEAT", "NONE");
            r->action_code = ACTION_SINGLE_SHAKE_REPEAT;
        }
        s.last_shake = now;
    } else if (!shake) {
        s.shake_high = false;
    }

    if (r->response_event) return;

    bool tilt = x->gyro_mag_dps > APP_GESTURE_GYRO_DPS &&
                (fabsf(x->roll_deg) > APP_GESTURE_TILT_DEG || fabsf(x->pitch_deg) > APP_GESTURE_TILT_DEG);
    if (tilt && cooldown && !s.gesture_active) {
        s.gesture_active = true;
        s.gesture_started = now;
    }
    if (!tilt && s.gesture_active) {
        uint32_t duration = now - s.gesture_started;
        s.gesture_active = false;
        if (duration >= APP_GESTURE_MIN_DURATION_MS && duration <= APP_GESTURE_MAX_DURATION_MS) {
            s.last_gesture = now;
            if (fabsf(x->roll_deg) > fabsf(x->pitch_deg)) {
                if (x->roll_deg < 0) {
                    set_event(r, "LEFT", "I AM OK", s.confirmed ? "USER_OK" : "NONE");
                    r->action_code = ACTION_LEFT_OK;
                } else {
                    set_event(r, "RIGHT", "I NEED HELP", s.confirmed ? "USER_NEEDS_HELP" : "NONE");
                    r->action_code = ACTION_RIGHT_HELP;
                }
            } else if (x->pitch_deg < 0) {
                set_event(r, "FORWARD", "SELECT/CONFIRM", "NONE");
                r->action_code = ACTION_FORWARD_CONFIRM;
            } else {
                set_event(r, "BACKWARD", "CANCEL/BACK", "NONE");
                r->action_code = ACTION_BACKWARD_CANCEL;
            }
            if (s.confirmed && x->roll_deg < 0) {
                s.confirmed = false;
                s.response_reported = false;
                r->system_state = SYS_USER_OK;
            } else if (s.confirmed) {
                r->system_state = SYS_NEEDS_HELP;
            }
        }
    }
}
