# Board

This is based on the ESP32-PICO-KIT V4.

# How to compile? (Windows)

- Open ESP-IDF PowerShell
- cd ThermostatConnecte/ESP32
- idf.py set-target esp32
- idf.py build

# Configure the project
- idf.py menuconfig
- Set your WIFI password / SSID
- Save with S

# How to flash? (Windows)

- Open the Windows device manager and find the line Silicon Labs CP210x to UART Bridge (COM??)
- COM??? is the COM port number, i.e., COM5. We name it [PORT].
- Open ESP-IDF PowerShell
- idf.py -p [PORT] flash

# Debug Console
- idf.py -p [PORT] monitor

