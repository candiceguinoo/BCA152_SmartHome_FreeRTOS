#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int last_read;           // last raw level seen (1 = released, 0 = pressed)
    int last_stable;         // last accepted level
    uint32_t last_change_ms; // time the raw level last changed
    uint32_t debounce_ms;    // how long a level must stay the same
} debounce_t;

void debounce_init(debounce_t *d, uint32_t debounce_ms, uint32_t now_ms);

// Feed one raw sample. Returns true exactly once per accepted press
// (stable level changed from released to pressed).
bool debounce_update(debounce_t *d, int raw_level, uint32_t now_ms);

#ifdef __cplusplus
}
#endif