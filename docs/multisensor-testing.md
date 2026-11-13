### Alexa and Automation

- Alexa successfully detected both **Plug** and **Greenhouse Sensors** through ESP RainMaker.
- Asked Alexa for the temperature, and it responded with approximately **29 Â°C** during testing.
- Humidity voice requests were not supported by the available Alexa interface.
- Temperature, humidity, and light readings were visible in the ESP RainMaker app.
- The LDR successfully detected changes in light intensity and reported the readings to RainMaker.
- Created an automation with the following condition and action:
  - **IF:** Greenhouse Sensors â†’ Light > 2
  - **THEN:** Plug â†’ Power = false (OFF)

**Actual Result:** PASSED. When the light reading went above 2, the automation sent an OFF command to the Smart Plug. The relay successfully switched OFF.

**Serial Monitor Evidence:**
```text
Received params: {"Plug":{"power":false}}
Found 1 params in write request for Plug
Reporting params: {"Plug":{"power":false}}
plug: Relay OFF
```

**Status: PASSED**
