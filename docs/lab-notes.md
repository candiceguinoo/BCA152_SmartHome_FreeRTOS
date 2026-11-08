# Lab Notes

## Hardware Substitutions

| Book | My hardware |
|---|---|
| ESP32-C3 DevKitM-1 (plug) | ESP32 DevKit V1 (ESP32-WROOM-32) |
| ESP32-S3 Box Lite | ESP32-S3 N16R8 (no built-in display) |
| BME280 | DHT11 |
| TSL2561 | Photoresistor (LDR) |

## Development Log

(Write what I tried, what failed, and what fixed it.)

- PlatformIO created a nested smart_switch/smart_switch folder. Moved files up one level.
  Build failed with FileNotFoundError on the old nested src path (stale .pio cache).
  Fixed by deleting .pio and reloading the window.

- LED did not blink on GPIO2 even though the serial log showed toggling. Moved the external
  LED to GPIO4 and it blinks correctly. GPIO2 is a boot-strapping pin, so it may be affected
  by the board's onboard circuitry.

- Relay task on GPIO19 (IN1 of a 2-channel module) toggles every 2 s. Serial log timestamps
  confirm 2000 ms period. Relay clicks match the log. Flash driver prints a harmless
  "generic chip" notice at boot.

- BOOT button (GPIO0) debounced in software with a 50 ms stable-time check.
  Tested on hardware: each press toggles the relay exactly once, no double toggles.

- Refactored: button_task now sends EVT_TOGGLE to a FreeRTOS queue (length 5).
  relay_task blocks on the queue and toggles the relay. Behavior on hardware unchanged.

- Added log_mutex (FreeRTOS mutex) and safe_log() so button_task and relay_task never
  print at the same time. Tested: one press gives "Button pressed" then "Relay ON/OFF".

## Multisensor
- Multisensor (ESP32-S3 N16R8, CH343 UART port on COM4) boots with 16 MB flash and custom
  partitions.csv. Heartbeat task prints every 1000 ms.

- DHT11 (3-pin module) on GPIO4 read with a self-contained bit-banged driver.
  Verified on hardware: 29 C / 74 % at rest, humidity rose to 93 % when breathed on.
  DHT11 resolution is 1 C, accuracy about +/- 2 C.

- LDR module (VCC, GND, AO, DO) with on-board divider. AO on GPIO5 (ADC1 ch4), DO unused.
  Ambient raw about 600-900 (intensity 14-21), torch raw about 2700-3700 (intensity 66-91).
  Brighter light gives higher raw, so mapping is direct (LDR_INVERTED = 0).
  DHT11 and LDR tasks run together without problems.

- Sensor data queue: dht_task and ldr_task send sensor_reading_t messages to a queue
  (length 10). report_task receives them and prints one combined line:
  "Temp | Humidity | Light". Verified on hardware.

- Serial output on multisensor is protected by log_mutex through safe_log(), so lines
  from different tasks never interleave.

- State machine (state_machine.c) is a pure function: INIT, NORMAL, WARNING, FAULT.
  Event group bits: BIT_DHT_OK, BIT_LDR_OK, BIT_DHT_FAULT, BIT_LIMIT_EXCEEDED,
  BIT_LIMIT_CLEARED. state_task owns the state and wakes only when a bit is set.
  Tested: INIT -> NORMAL at startup (327 ms). Pulling the DHT11 data wire gave 3
  failed reads, then NORMAL -> FAULT. Replugging gave FAULT -> NORMAL.
  FAULT needs 3 failed reads in a row because single DHT11 failures are normal.
  Humidity thresholds use hysteresis (85 % on, 80 % off).

- reated debounce.c/h in multisensor/src by mistake. Moved them to smart_switch/src before committing
## Troubleshooting

- PlatformIO created a nested smart_switch/smart_switch folder. Moved the files up one level.
- Stale .pio cache caused FileNotFoundError on the old path. Deleted .pio and rebuilt.
- pio was not recognized in new terminals. Fixed with Set-Alias or the PlatformIO terminal.
- Serial monitor showed garbage at 9600 baud. Fixed with -b 115200.
- LED on GPIO2 did not blink. Moved the LED to GPIO4.
- Build failed: missing partition table default_16MB.csv. Created my own partitions.csv.
- Upload failed with "could not open port COMx" because I used the placeholder. Used pio device list.
- Upload failed with "No serial data received". Fixed by holding BOOT, tapping EN, releasing BOOT.
- ESP32-S3 disappeared from the COM list. Fixed by replugging into the UART port.
- Generated sdkconfig.* files were added to .gitignore.

- Warning test: the air-conditioned room kept humidity at 36-47 %, so I temporarily lowered
  HUMIDITY_LIMIT to 55 % and HUMIDITY_LIMIT_CLEAR to 50 %. Breathing on the DHT11 raised it
  to 68-78 % and the state went NORMAL -> WARNING. After it dried, humidity fell to 49 % and
  the state returned WARNING -> NORMAL. Restored the limits to 85 % / 80 %.

- Unit tests (Unity, native env): 9 state machine + 6 light map tests, 15 of 15 passed.
- Moved multisensor headers into include/. Native build flags (-Iinclude) and the ESP-IDF
  build both still work. Board image builds successfully.