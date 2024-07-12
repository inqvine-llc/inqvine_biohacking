# Inqvine Biohacking

This repository contains demo projects involving various AI and biohacking kits. Each demo showcases different aspects of integrating biometric sensors with AI and hardware components.

## Hardware Overview

### E3K Module
The E3K module is a versatile biometric sensing platform. It includes:
- BNO080 IMU (Inertial Measurement Unit)
- Multiple analog input channels for various biometric sensors
- Bluetooth Low Energy (BLE) capability for wireless communication

Key features:
- 6 analog input channels (36, 39, 32, 34, 35, 33)
- Configurable sampling rates (10Hz, 100Hz, 500Hz, 1000Hz)
- Real-time data streaming over BLE

### ESP32 Board
The ESP32 is a powerful, low-cost microcontroller with built-in Wi-Fi and Bluetooth capabilities. In this project, it's used to:
- Receive BLE data from the E3K module
- Control a servo motor based on the received data

### Servo Motor
A standard servo motor is used for demonstration purposes. It's controlled by the ESP32 based on the IMU data received from the E3K module.

## Demo 1: E3K Module IMU to Servo Control via BLE

This demo demonstrates how to connect an E3K module's IMU to a servo motor using Bluetooth Low Energy (BLE) communication.

### Components Used:
- E3K module with IMU
- ESP32 board
- Servo motor (e.g., SG90 or similar)
- Jumper wires

### Connections:
- Servo motor:
  - Signal wire to ESP32 GPIO 15
  - Power (5V) and Ground to appropriate ESP32 pins

### File Structure:
- `e3k_ble_client.py`: Python script for BLE communication with the E3K module
- `esp32_ble_servo_control.ino`: Arduino sketch for the ESP32 to control the servo based on IMU data
- `e3k_ble_server.ino`: Arduino sketch for the E3K module to send IMU data over BLE

### Setup and Usage:

1. Upload the `e3k_ble_server.ino` sketch to your E3K module.
2. Upload the `esp32_ble_servo_control.ino` sketch to your ESP32 board.
3. Install the required Python libraries: `pip install asyncio bleak`
4. Run the Python script: `python e3k_ble_client.py`
The Python script will scan for the E3K module, connect to it, and start receiving IMU data. The ESP32 will then use this data to control the servo motor's position.

### How it Works:

1. The E3K module reads IMU data and sends it over BLE.
2. The Python script acts as a BLE client, receiving and parsing the IMU data.
3. The ESP32 acts as another BLE client, receiving the IMU data and using it to control the servo motor.

## Future Demos

More demos will be added to this repository, showcasing different aspects of AI and biohacking integration. Stay tuned for updates!

## Contributing

We welcome contributions to this project. Please feel free to submit issues, fork the repository and send pull requests!

## License

This project is open source and available under the [MIT License](https://opensource.org/licenses/MIT).

## Disclaimer

This project is for educational and research purposes only. Always consult with healthcare professionals before using any biohacking techniques or devices. Use caution when working with electronic components and follow proper safety procedures.