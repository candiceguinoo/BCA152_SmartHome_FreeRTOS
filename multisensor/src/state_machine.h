#pragma once

typedef enum {
    STATE_INIT,
    STATE_NORMAL,
    STATE_WARNING,
    STATE_FAULT
} system_state_t;

typedef enum {
    SM_EVT_SENSORS_READY,
    SM_EVT_THRESHOLD_EXCEEDED,
    SM_EVT_VALUES_NORMAL,
    SM_EVT_SENSOR_FAULT,
    SM_EVT_SENSOR_RECOVERED
} sm_event_t;

// Pure function: returns the next state. No hardware access.
system_state_t sm_next_state(system_state_t current, sm_event_t evt);

const char *sm_state_name(system_state_t state);