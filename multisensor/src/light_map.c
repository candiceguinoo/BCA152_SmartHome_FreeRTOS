#include "light_map.h"

int light_map_percent(int raw, int inverted)
{
    if (raw < 0)    raw = 0;
    if (raw > 4095) raw = 4095;

    int percent = raw * 100 / 4095;
    return inverted ? 100 - percent : percent;
}