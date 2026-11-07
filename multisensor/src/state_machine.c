#include "state_machine.h"

system_state_t sm_next_state(system_state_t current, sm_event_t evt)
{
    switch (current) {
    case STATE_INIT:
        if (evt == SM_EVT_SENSORS_READY) return STATE_NORMAL;
        if (evt == SM_EVT_SENSOR_FAULT)  return STATE_FAULT;
        break;
    case STATE_NORMAL:
        if (evt == SM_EVT_THRESHOLD_EXCEEDED) return STATE_WARNING;
        if (evt == SM_EVT_SENSOR_FAULT)       return STATE_FAULT;
        break;
    case STATE_WARNING:
        if (evt == SM_EVT_VALUES_NORMAL) return STATE_NORMAL;
        if (evt == SM_EVT_SENSOR_FAULT)  return STATE_FAULT;
        break;
    case STATE_FAULT:
        if (evt == SM_EVT_SENSOR_RECOVERED) return STATE_NORMAL;
        break;
    }
    return current;   // any other event is ignored
}

const char *sm_state_name(system_state_t state)
{
    switch (state) {
    case STATE_INIT:    return "INIT";
    case STATE_NORMAL:  return "NORMAL";
    case STATE_WARNING: return "WARNING";
    case STATE_FAULT:   return "FAULT";
    }
    return "UNKNOWN";
}