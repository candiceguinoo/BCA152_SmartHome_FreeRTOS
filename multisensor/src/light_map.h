#pragma once

// Maps a 12-bit ADC reading (0-4095) to 0-100. If inverted, brighter = lower raw.
int light_map_percent(int raw, int inverted);