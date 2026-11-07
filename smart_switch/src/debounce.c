#include "debounce.h"

void debounce_init(debounce_t *d, uint32_t debounce_ms, uint32_t now_ms)
{
    d->last_read = 1;
    d->last_stable = 1;
    d->last_change_ms = now_ms;
    d->debounce_ms = debounce_ms;
}

bool debounce_update(debounce_t *d, int raw_level, uint32_t now_ms)
{
    if (raw_level != d->last_read) {
        d->last_read = raw_level;
        d->last_change_ms = now_ms;
    }

    if ((now_ms - d->last_change_ms) > d->debounce_ms &&
        raw_level != d->last_stable) {
        d->last_stable = raw_level;
        if (d->last_stable == 0) {
            return true;    // pressed
        }
    }
    return false;
}