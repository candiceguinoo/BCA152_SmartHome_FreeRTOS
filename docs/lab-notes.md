# Lab Notes

Development log for the BCA152 Smart Home and FreeRTOS Multisensor project. It records the problems I encountered, what I tried, and how I fixed them. Formal test results are in [smart-plug-testing.md](smart-plug-testing.md) and [multisensor-testing.md](multisensor-testing.md).

## 1. Hardware substitutions

| Book | My hardware | Notes |
|---|---|---|
| ESP32-C3 DevKitM-1 (plug) | ESP32 DevKit V1 (ESP32-WROOM-32) | 4 MB flash, relay on GPIO19 |
| ESP32-S3 Box Lite (multisensor) | ESP32-S3 N16R8 | 16 MB flash, no built-in display |
| BME280 | DHT11 (3-pin module) | Temperature and humidity only |
| TSL2561 | LDR module (VCC, GND, AO, DO) | AO connected to the ADC, DO unused |
| 1-channel relay | 2-channel relay module | Only IN1 is used, active-low |

## 2. Project setup

- Created the repository with a README and `.gitignore`, then created separate PlatformIO projects for `smart_switch` and `multisensor` using ESP-IDF.
- PlatformIO created a nested `smart_switch/smart_switch` folder. I moved the files up one level. Deleting the old `.pio` cache fixed the build error caused by the previous path.
- When `pio` was not recognized in new terminals, I used the PlatformIO executable or configured a PowerShell alias.
- Added generated `sdkconfig.*` files to `.gitignore`.
- Fixed unreadable serial output by using 115200 baud instead of 9600.
- Used `pio device list` to identify the correct COM ports. COM4 belongs to the ESP32-S3 multisensor board, while COM7 belongs to the smart switch.
- Fixed an upload issue by holding BOOT, tapping EN, and releasing BOOT.
- Reconnected the ESP32-S3 using its UART port and a data-capable USB cable when it disappeared from the COM list.
- Moved `debounce.c` and `debounce.h` from the wrong project folder into `smart_switch/src`.

## 3. Smart Plug (`smart_switch`, ESP32 DevKit V1)

### FreeRTOS firmware

- **LED:** The external LED did not work as expected on GPIO2, so I moved it to GPIO4. It blinked correctly after the change.
- **Relay:** The relay task on GPIO19 toggled every 2 seconds. The serial timestamps and relay clicks confirmed the timing.
- **Button:** The BOOT button on GPIO0 uses a 50 ms debounce check. Each press toggles the relay once.
- **Queue:** `button_task` sends toggle events through a FreeRTOS queue, and `relay_task` receives them and controls the relay.
- **Mutex:** `log_mutex` and `safe_log()` prevent task messages from being printed at the same time.
- **Debounce module:** Moved the debounce logic into `debounce.c` so it could be tested on the PC. All 6 debounce unit tests passed.
- **Partitions:** Added a custom partition table with two OTA application slots and configured the board for 4 MB flash.

### RainMaker integration

- Added ESP RainMaker v2.1.0 as a Git submodule and created the shared `ch9_common` component.
- Updated the project for the newer RainMaker version, including the use of `app_network` and `esp_rmaker_ota_enable_default()`.
- Registered the Smart Plug as a standard `esp.device.plug` device with a power parameter.
- Physical button presses and RainMaker commands use the same queue. The relay state is reported to RainMaker after changes.
- Successfully controlled the plug through the ESP RainMaker app.
- Successfully tested Alexa voice commands to turn the plug ON and OFF.
- Tested RainMaker schedules and scenes successfully.
- Completed the Smart Plug OTA update to firmware version 1.0.1. The firmware was uploaded to RainMaker, the OTA job completed, and the device rebooted and reconnected. Remote control still worked after the update.

### Automation

- Created an automation using the Greenhouse Sensors light reading:
  - **IF:** Greenhouse Sensors → Light > 2
  - **THEN:** Plug → Power = false (OFF)
- The automation successfully sent the OFF command, and the relay switched OFF.

## 4. Multisensor (`multisensor`, ESP32-S3 N16R8)

### Board bring-up

- Configured the board for 16 MB flash and created a custom `partitions.csv`.
- Fixed the missing partition table error by creating the required partition file.
- Changed the flash configuration from 4 MB to 16 MB in `sdkconfig.defaults`, removed the generated SDK configuration, and rebuilt the project.
- Used a heartbeat task to confirm that the board was running and serial output was working.

### Sensors

- **DHT11:** Used a self-contained bit-banged driver with checksum validation.
- The DHT11 DATA wire was initially connected to GPIO4. After troubleshooting, I moved it to GPIO5.
- **Final DHT11 wiring:** VCC → 3.3V, DATA → GPIO5, GND → GND.
- **LDR:** The LDR signal wire is connected to GPIO4. Its digital output is unused.
- Both sensors were tested together and successfully read temperature, humidity, and light values.

### FreeRTOS design

- **Queue:** `dht_task` and `ldr_task` send `sensor_reading_t` messages to `report_task`.
- **Mutex:** `log_mutex` and `safe_log()` prevent messages from different tasks from interleaving.
- **State machine:** `state_machine.c` handles INIT, NORMAL, WARNING, and FAULT states.
- **Event group:** Uses `BIT_DHT_OK`, `BIT_LDR_OK`, `BIT_DHT_FAULT`, `BIT_LIMIT_EXCEEDED`, and `BIT_LIMIT_CLEARED`.
- Tested sensor fault handling by disconnecting the DHT11 DATA wire. Three failed reads caused the state to change from NORMAL to FAULT. Reconnecting the wire returned it to NORMAL.
- Tested the WARNING state by temporarily lowering the humidity limits and exposing the sensor to my breath. The state changed from NORMAL to WARNING and returned to NORMAL after the humidity decreased.
- Restored the humidity limits to 85% for the warning threshold and 80% for clearing the warning.

### RainMaker integration

- Registered the device as **Greenhouse Sensors**.
- Added temperature, humidity, and light intensity parameters.
- Configured the device to report sensor readings every 5 seconds.
- BLE provisioning, Wi-Fi connection, and RainMaker registration worked.
- Temperature, humidity, and light readings appeared in the ESP RainMaker app.
- Fixed the missing factory NVS partition by adding the `fctry` partition.
- Erased old Wi-Fi provisioning data and provisioned the device again.
- Fixed the DHT11 timeout by checking the wiring and moving its DATA connection to GPIO5.
- Fixed a NimBLE host task stack overflow by increasing `CONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE` from 4096 to 8192.
- Sensor readings were still reported even when the RainMaker time-series warning appeared.

## 5. Build problems from RainMaker integration

| Problem | Cause | Fix |
|---|---|---|
| Version solving failed | The component path included the Matter controller | Listed only the required RainMaker components |
| Main target not found | `EXTRA_COMPONENT_DIRS` overwrote PlatformIO's source directory | Used `list(APPEND ...)` |
| RainMaker certificate not found | ESP-IDF embedded certificate files were not included | Added the required certificates to `board_build.embed_txtfiles` |
| C++ compiler flags used for C files | PlatformIO build issue after adding C++ files | Converted the firmware to plain C |
| Multisensor depended on another project's components | The certificate path pointed to `smart_switch` | Used the multisensor project's own managed components |

## 6. Testing and analysis

- **Unit tests:** All 15 multisensor tests passed: 9 state machine tests and 6 light mapping tests. All 6 smart switch debounce tests also passed.
- **Static analysis:** `pio check` with cppcheck reported no defects in either project.
- **Hardware tests:** The Smart Plug and Multisensor hardware test results are documented in their separate testing reports.

## 7. Alexa

- Alexa successfully detected **Plug** and **Greenhouse Sensors** after account linking.
- Voice commands to turn the Smart Plug ON and OFF both worked.
- Asking Alexa for the temperature worked and returned approximately 29°C during testing.
- Humidity voice requests were not supported by the available Alexa interface.
- Temperature, humidity, and light readings were visible in the ESP RainMaker app.

## 8. Cleanup

- Removed unused C++ node classes after converting the firmware to plain C.
- Removed a VS Code workspace file from `multisensor/src` because it was being treated as a source file.
- Removed the local backup of the pre-RainMaker files. The Git history keeps the earlier code.

## 9. Remaining limitations

- The Smart Plug has no separate state machine or event group implementation. These are implemented in the multisensor project.
- Multisensor OTA was not tested. The completed OTA test was for the Smart Plug.
- The available Alexa interface did not support humidity voice requests.