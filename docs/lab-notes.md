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
