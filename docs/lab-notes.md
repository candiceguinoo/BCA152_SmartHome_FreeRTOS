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

MULTISENSOR
- Multisensor (ESP32-S3 N16R8, CH343 UART port on COM4) boots with 16 MB flash and custom
  partitions.csv. Heartbeat task prints every 1000 ms.

- DHT11 (3-pin module) on GPIO4 read with a self-contained bit-banged driver.
  Verified on hardware: 29 C / 74 % at rest, humidity rose to 93 % when breathed on.
  DHT11 resolution is 1 C, accuracy about +/- 2 C.

- LDR module (VCC, GND, AO, DO) with on-board divider. AO on GPIO5 (ADC1 ch4), DO unused.
  Ambient raw about 600-900 (intensity 14-21), torch raw about 2700-3700 (intensity 66-91).
  Brighter light gives higher raw, so mapping is direct (LDR_INVERTED = 0).
  DHT11 and LDR tasks run together without problems.