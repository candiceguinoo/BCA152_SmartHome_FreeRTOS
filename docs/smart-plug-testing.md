# Smart Plug Testing

## Test 1: Physical Button and Relay

**Procedure:** Pressed the ESP32 BOOT button to control the relay.

**Expected Result:** Each button press should toggle the relay once.

**Actual Result:** PASSED. The button successfully controlled the relay. The debounce logic prevented repeated toggles from a single press.

**Status: PASSED**

## Test 2: FreeRTOS Queue and Relay Task

**Procedure:** Tested the button task sending toggle events through a FreeRTOS queue to the relay task.

**Expected Result:** The relay task should receive the event and change the relay state.

**Actual Result:** PASSED. The relay responded to the queued button events.

**Status: PASSED**

## Test 3: ESP RainMaker Control

**Procedure:** Connected the Smart Plug to ESP RainMaker and used the mobile application to turn it ON and OFF.

**Expected Result:** The relay should follow the power commands from the application.

**Actual Result:** PASSED. The Smart Plug responded to RainMaker commands, and the relay state was updated.

**Status: PASSED**

## Test 4: Alexa Voice Control

**Procedure:** Connected Amazon Alexa with ESP RainMaker. Used the voice commands "Alexa, turn on the plug" and "Alexa, turn off the plug".

**Expected Result:** The Smart Plug should respond to Alexa voice commands, and the relay should switch ON or OFF depending on the requested power state.

**Actual Result:** PASSED. Alexa successfully controlled the Smart Plug using both voice commands. The relay switched ON when the turn-on command was given and switched OFF when the turn-off command was given.

**Status: PASSED**

## Test 5: Schedules and Scenes

**Procedure:** Created and tested schedules and scenes through the ESP RainMaker application.

**Expected Result:** The configured schedules and scenes should control the Smart Plug.

**Actual Result:** PASSED. The schedules and scenes were successfully tested.

**Status: PASSED**

## Test 6: Light-Based Automation

**Procedure:** Created an ESP RainMaker automation using the Greenhouse Sensors light reading.

- **IF:** Greenhouse Sensors → Light > 2
- **THEN:** Plug → Power = false (OFF)

**Expected Result:** When the light reading goes above 2, the automation should send an OFF command to the Smart Plug.

**Actual Result:** PASSED. The automation sent the OFF command, and the relay switched OFF.

**Serial Monitor Evidence:**

```text
Received params: {"Plug":{"power":false}}
Found 1 params in write request for Plug
Reporting params: {"Plug":{"power":false}}
plug: Relay OFF
```

**Status: PASSED**

## Test 7: Over-the-Air (OTA) Update

**Procedure:** Built firmware version 1.0.1, uploaded the firmware file to the ESP RainMaker dashboard, and started an OTA job for the Smart Plug.

**Expected Result:** The device should download and verify the firmware, complete the update, reboot, reconnect, and continue working.

**Actual Result:** PASSED. The device received the OTA update and displayed `OTA Upgrade finished and verified successfully`. It rebooted and reconnected to RainMaker. ON/OFF remote control still worked after the update.

**Status: PASSED**