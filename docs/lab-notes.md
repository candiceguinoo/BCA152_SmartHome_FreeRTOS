# Lab Notes

Development log for the BCA152 Smart Home and FreeRTOS Multisensor project. It records what I
tried, what failed and what fixed it, in the order of the project. The formal test results are
in [smart-plug-testing.md](smart-plug-testing.md) and
[multisensor-testing.md](multisensor-testing.md).

## 1. Hardware substitutions

| Book | My hardware | Notes |
|---|---|---|
| ESP32-C3 DevKitM-1 (plug) | ESP32 DevKit V1 (ESP32-WROOM-32) | 4 MB flash, relay on GPIO19 |
| ESP32-S3 Box Lite (multisensor) | ESP32-S3 N16R8 | 16 MB flash, no built-in display |
| BME280 | DHT11 (3-pin module) | Temperature and humidity only, 1 °C resolution, about ±2 °C |
| TSL2561 | LDR module (VCC, GND, AO, DO) | AO on the ADC, DO unused |
| Relay (1-channel) | 2-channel relay module | Only IN1 is used, active-low |

## 2. Project setup

- Created the repository with a README and `.gitignore`, then one PlatformIO project per
  device (`smart_switch` and `multisensor`) using the ESP-IDF framework.
- PlatformIO created a nested `smart_switch/smart_switch` folder. I moved the files up one
  level. The build then failed with `FileNotFoundError` on the old path because of a stale
  `.pio` cache. Deleting `.pio` and rebuilding fixed it.
- `pio` was not recognized in new terminals. I used `Set-Alias` or the PlatformIO terminal.
- Generated `sdkconfig.*` files were added to `.gitignore`.
- The serial monitor showed garbage when it opened at 9600 baud. Fixed with `-b 115200`.
- Several upload errors came from the COM port: `could not open port COMx` was my own
  placeholder, so I used `pio device list` to find the real port. COM4 belongs to the ESP32-S3,
  so I never uploaded the smart switch to it.
- `No serial data received` on upload was fixed by holding BOOT, tapping EN and releasing BOOT.
- The ESP32-S3 once disappeared from the COM list. Replugging it into the UART port with a
  data cable fixed it.
- I created `debounce.c/h` in `multisensor/src` by mistake and moved them to
  `smart_switch/src` before committing.

## 3. Smart Plug (`smart_switch`, ESP32 DevKit V1)

### FreeRTOS firmware

- **LED:** nothing blinked on GPIO2 even though the serial log showed the toggling. I moved the
  external LED to GPIO4 and it blinked correctly. GPIO2 is a boot-strapping pin, so the board's
  onboard circuitry may affect it.
- **Relay:** the first relay task on GPIO19 (IN1 of the 2-channel module) toggled every 2 s.
  The serial timestamps confirmed the 2000 ms period and the clicks matched the log. The flash
  driver prints a harmless "generic chip" notice at boot.
- **Button:** the BOOT button (GPIO0) is debounced in software with a 50 ms stable-time check.
  Each press toggles the relay exactly once.
- **Queue:** `button_task` sends a toggle event to a FreeRTOS queue and `relay_task` receives it
  and drives the relay.
- **Mutex:** `log_mutex` and `safe_log()` stop `button_task` and `relay_task` from printing at
  the same time.
- **Debounce module:** the debounce logic was moved into `debounce.c` with a timestamp
  parameter so it can be tested on the PC (6 unit tests). Verified on the DevKit V1 that one
  press gives one `Button pressed` and one relay toggle.
- **Partitions:** the default partition table has one 1 MB app slot and no OTA. I added a custom
  `partitions.csv` with two OTA slots and set the flash size to 4 MB in `sdkconfig.defaults`.

### RainMaker integration

- Added ESP RainMaker v2.1.0 as a git submodule and a shared component `ch9_common`
  (`app_driver`, `app_node`).
- Chapter 9 targets an older RainMaker. In v2.1.0 `app_wifi` is replaced by `app_network`, the
  OTA call is `esp_rmaker_ota_enable_default()`, and a timezone service exists. I compared my
  code with the `switch` example from the submodule and updated `ch9_common`.
- The plug is a standard `esp.device.plug` with a power parameter. Button presses and RainMaker
  commands both go through the same queue, and the relay state is reported to RainMaker on
  every change.
- OTA: built firmware version 1.0.1 (`version.txt`), uploaded `firmware.bin` to the RainMaker
  dashboard and ran an OTA job. The update downloaded over HTTPS, was verified, and the device
  rebooted and reconnected. Remote control still worked afterwards.
- Alexa plug voice control: <result: did "turn on / off the plug" work?>
- Automation (light above 40 turns the plug off): <result>

## 4. Multisensor (`multisensor`, ESP32-S3 N16R8)

### Board bring-up

- The board boots with 16 MB flash and a custom `partitions.csv`. A heartbeat task printed every
  1000 ms to confirm the upload and serial output.
- First build failed with `Missing partition table file default_16MB.csv`. Under ESP-IDF,
  PlatformIO does not provide the Arduino partition files, so I wrote my own `partitions.csv`.
- `sdkconfig.defaults` first had `FLASHSIZE_4MB` (copied from the DevKit V1). The N16R8 has
  16 MB, so I set `FLASHSIZE_16MB`, deleted the generated sdkconfig and rebuilt.
- After that rebuild the DHT11 timed out and the LDR read random values (a floating pin). The
  cause was loose breadboard wiring, and reseating the wires fixed both sensors.

### Sensors

- **DHT11 (GPIO4):** read with a self-contained bit-banged driver with checksum validation
  (`esp-idf-lib` was too heavy under PlatformIO). Verified on hardware: 29 °C and 74 % at rest,
  and humidity rose to 93 % when I breathed on it. A 3-pin module needed its pin order checked
  before wiring.
- **LDR (GPIO5, ADC1 channel 4):** ambient raw is about 600-900 (intensity 14-21) and torch raw
  about 2700-3700 (intensity 66-91). Brighter light gives a higher value, so the mapping is
  direct (`LDR_INVERTED = 0`). The mapping lives in `light_map.c` so it can be unit tested.
- Both tasks run together without problems.

### FreeRTOS design

- **Queue:** `dht_task` and `ldr_task` send `sensor_reading_t` messages (queue length 10) to
  `report_task`, which prints one combined `Temp | Humidity | Light` line.
- **Mutex:** `log_mutex` and `safe_log()` keep lines from different tasks from interleaving.
- **State machine** (`state_machine.c`, a pure function): INIT, NORMAL, WARNING, FAULT.
- **Event group** bits: `BIT_DHT_OK`, `BIT_LDR_OK`, `BIT_DHT_FAULT`, `BIT_LIMIT_EXCEEDED`,
  `BIT_LIMIT_CLEARED`. `state_task` owns the state and wakes only when a bit is set.
- **State tests on hardware:**
  - INIT to NORMAL at startup (327 ms).
  - Pulling the DHT11 data wire gave 3 failed reads, then NORMAL to FAULT. Replugging gave
    FAULT to NORMAL. FAULT needs 3 failed reads in a row because a single DHT11 failure is normal.
  - Warning test: the air-conditioned room kept humidity at 36-47 %, so I temporarily lowered
    the limits to 55 % and 50 %. Breathing on the sensor raised it to 68-78 % and the state went
    NORMAL to WARNING. After it dried, humidity fell to 49 % and the state returned to NORMAL.
    I restored the limits to 85 % (on) and 80 % (off), which gives hysteresis.

### RainMaker integration

- The device is registered as **Greenhouse Sensors** with the standard temperature parameter, a
  humidity parameter (`esp.param.humidity`) and a light intensity parameter. Values are reported
  every 5 s, and BLE provisioning, Wi-Fi and device registration all worked.
- Problems and fixes:
  - Missing factory NVS partition: added the `fctry` partition to the partition table.
  - Old Wi-Fi provisioning data: erased the flash and provisioned again.
  - DHT11 `ESP_ERR_TIMEOUT`: checked and adjusted the wiring.
  - NimBLE host task stack overflow during RainMaker authentication: raised
    `CONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE` from 4096 to 8192 in `sdkconfig.defaults`.
  - RainMaker time-series warning: readings were still reported and displayed even though time
    sync was not available yet.
- Multisensor OTA: <result, or "not tested">

## 5. Build problems from the RainMaker integration

These came up while getting RainMaker v2.1.0 to build under PlatformIO (ESP-IDF 6.0.1).

| Problem | Cause | Fix |
|---|---|---|
| `Version solving failed` (esp_matter vs json_parser) | `EXTRA_COMPONENT_DIRS` pointed at the whole `examples/common` folder, which includes the Matter controller | Listed only `rmaker_app_network` and `rmaker_app_insights` |
| `Couldn't find the main target of the project!` | My `set(EXTRA_COMPONENT_DIRS ...)` overwrote PlatformIO's `src` entry, so the `src` component was never built | Used `list(APPEND ...)` |
| `rmaker_mqtt_server.crt.S not found` | Known PlatformIO issue with files embedded by ESP-IDF components | Listed the RainMaker and Insights certificates in `board_build.embed_txtfiles` |
| `-fuse-cxa-atexit is valid for C++ but not for C` | Known PlatformIO 7.0.x bug: C++ flags leak into C compiles once the project has a C++ file | Converted the firmware from C++ classes to plain C |
| Multisensor build depended on `smart_switch/managed_components` for certificates | A fresh clone would not have that folder | Pointed the multisensor at its own `managed_components` |

## 6. Testing and analysis

- **Unit tests** (Unity, native environment): 15 of 15 multisensor tests (9 state machine and 6
  light mapping) and 6 of 6 smart switch tests (debounce) pass. They were re-run after the
  RainMaker changes. A first run failed with `Nothing to build` because my test file was named
  `.h` instead of `.c`.
- **Moving headers into `include/`:** the native build needed `-Iinclude` and the ESP-IDF build
  needed `../include` in its include path. Both still work.
- **Static analysis:** `pio check` with cppcheck (warning, style and performance checks)
  reported no defects in either project. The native environment is ignored by `pio check`, as
  expected.
- **Hardware tests:** see the two test reports.

## 7. Alexa

- Alexa lists **Plug** and **Greenhouse Sensors** after account linking.
- Asking for the temperature works (29 °C in my test). Asking for the humidity answers that it
  is not supported. Alexa has a humidity interface, but it defines no voice utterances, and I
  found no ambient-light interface. All three values are visible in the RainMaker app.

## 8. Cleanup before the merge

- Removed the unused C++ node classes, since the firmware is now plain C.
- Removed a VS Code workspace file from `multisensor/src`, where the CMake glob treated it as a
  source file.
- Removed the local backup of the pre-RainMaker files. The Git history keeps that code.

## 9. Open items

- Automation and Alexa plug voice results (section 3).
- Multisensor OTA result (section 4).
- Smart switch state machine and event group: only the multisensor has them. <decide: add them
  or leave them out>