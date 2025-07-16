# dht-iot-blynk-web-server

/*
 * -----------------------------------------------------------------------------
 * Project: ESP32 DHT11 Web Server with I2C LCD and Blynk Integration
 *
 * Description:
 * This code turns an ESP32 into a multi-functional sensor hub.
 * 1. Connects to your home Wi-Fi to send data to the Blynk cloud (STA mode).
 * 2. Creates its own backup Wi-Fi network (AP mode).
 * 3. Reads temperature and humidity from a DHT11 sensor.
 * 4. Serves a web page to display readings locally.
 * 5. Displays readings on a 16x2 I2C LCD.
 * 6. Sends readings to Blynk: Temperature (V0), Humidity (V1).
 * 7. Triggers a Blynk event "temp-max" if temperature exceeds 30°C.
 *
 * Hardware Required:
 * - ESP32 development board
 * - DHT11 temperature and humidity sensor
 * - 16x2 I2C LCD Display
 * - Jumper wires
 *
 * Wiring: (No changes from previous version)
 * - DHT11 Sensor: VCC -> 3.3V, GND -> GND, Data -> GPIO 19
 * - I2C LCD Display: VCC -> VIN/5V, GND -> GND, SDA -> GPIO 21, SCL -> GPIO 22
 *
 * Libraries to Install via Arduino IDE Library Manager:
 * - "DHT sensor library" by Adafruit
 * - "Adafruit Unified Sensor" by Adafruit
 * - "LiquidCrystal I2C" by Frank de Brabander
 * - "Blynk" by Volodymyr Shymanskyy
 * -----------------------------------------------------------------------------
*/