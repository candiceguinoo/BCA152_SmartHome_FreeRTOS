# Multisensor Testing

## DHT11 and LDR Sensor Testing

**Hardware connections:**
- DHT11 VCC → 3.3V
- DHT11 DATA → GPIO5
- DHT11 GND → GND
- LDR signal → GPIO4

**Test Results:**

- DHT11 successfully read temperature values.
- DHT11 successfully read humidity values.
- LDR successfully detected changes in light intensity.
- Both sensors successfully sent their readings to the ESP RainMaker mobile application.
- Temperature, humidity, and light readings were visible in the app.

**Sample Serial Monitor Output:**

```text
Reporting params: {"Greenhouse Sensors":{"Temperature":30.00000,"Humidity":67.00000,"Light":100}}
multisensor: RainMaker readings reported
multisensor: Temp: 30 C | Humidity: 67 % | Light: 100
```

**Actual Result:** PASSED. Both sensors worked correctly after moving the DHT11 DATA connection from GPIO4 to GPIO5. The ESP32-S3 successfully read the sensor values and displayed them in the ESP RainMaker app.

**Status: PASSED**

## Alexa and Automation

- Alexa successfully detected both **Plug** and **Greenhouse Sensors** through ESP RainMaker.
- Asked Alexa for the temperature, and it responded with approximately **29°C** during testing.
- Humidity voice requests were not supported by the available Alexa interface.
- Temperature, humidity, and light readings were visible in the ESP RainMaker app.
- The LDR successfully detected changes in light intensity and reported the readings to RainMaker.
- Created an automation with the following condition and action:
  - **IF:** Greenhouse Sensors → Light > 2
  - **THEN:** Plug → Power = false (OFF)

**Actual Result:** PASSED. When the light reading went above 2, the automation sent an OFF command to the Smart Plug. The relay successfully switched OFF.

**Serial Monitor Evidence:**

```text
Received params: {"Plug":{"power":false}}
Found 1 params in write request for Plug
Reporting params: {"Plug":{"power":false}}
plug: Relay OFF
```

**Status: PASSED**
