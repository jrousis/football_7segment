# football_7segment# Football 7-Segment Display

This project is a football scoreboard using an ESP32 microcontroller. It displays the score, time, and other relevant information on a 7-segment display. The project includes features such as WiFi connectivity, remote control, and automatic brightness adjustment.

## Features

- Display score and time on a 7-segment display
- WiFi connectivity for remote control and updates
- Automatic brightness adjustment using a photoresistor
- Remote control buttons for various functions
- EEPROM storage for WiFi credentials and settings
- Synchronize time with an NTP server
- External RTC for timekeeping

## Hardware Requirements

- ESP32 microcontroller
- 7-segment display
- DS18B20 temperature sensor
- DS1302 RTC module
- Photoresistor
- Buzzer
- Push buttons for control

## Software Requirements

- Arduino IDE
- ESP32 board support package
- Required libraries:
  - esp_now
  - esp32fota
  - WiFi
  - DNSServer
  - ESPUI
  - Rousis7segment
  - EEPROM
  - OneWire
  - DallasTemperature
  - Ds1302
  - Chrono

## Installation

1. Clone this repository to your local machine.
2. Open the project in Arduino IDE.
3. Install the required libraries using the Library Manager.
4. Connect your ESP32 to your computer.
5. Select the correct board and port in the Arduino IDE.
6. Upload the code to the ESP32.

## Usage

1. Power on the ESP32 and the connected hardware.
2. The 7-segment display will show the initial state.
3. Use the remote control buttons to start/stop the timer, adjust the score, and other functions.
4. Connect to the ESP32's WiFi hotspot to access the web interface for additional settings and controls.
5. The display will automatically adjust its brightness based on the ambient light.

## Configuration

- WiFi credentials and other settings are stored in the EEPROM.
- Use the web interface to update the settings and save them to the EEPROM.
- The time is synchronized with an NTP server and can be adjusted using the web interface.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgements

- [ESPUI](https://github.com/s00500/ESPUI) for the web interface library
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) for the temperature sensor library
- [Chrono](https://github.com/SofaPirate/Chrono) for the timer library
