# Autonomous Car Control

Autonomous Car Control provides the full software stack for a small self‑driving car based on an ESP32 microcontroller.  
The project contains firmware for the ESP32 that drives the motors and sensors, an Android application used for control and configuration, plus generated documentation and supporting images.

## Features

- **Motor and Steering Control** – PWM drivers for a DC motor and a servo used for steering.  
- **PID Speed Regulation** – configurable PID controller running in a dedicated FreeRTOS task.  
- **Sensor Support** – I²C drivers for devices such as the ADXL345 accelerometer, VL53L0X distance sensor and BME280 temperature sensor.  
- **Wi‑Fi Networking** – UDP communication with a companion Android application or a Jetson board.  
- **Autonomous and Diagnostic Modes** – drive the car autonomously based on camera/Jetson input or manually from the mobile app.

## Repository Layout

| Path | Description |
| ---- | ----------- |
| [`Car-Control/Car_Controll`](Car-Control/Car_Controll) | ESP‑IDF firmware project for the ESP32.  Main entry point in [`main.c`](Car-Control/Car_Controll/main/main.c). |
| [`HappySilicon`](HappySilicon) | Android application used to connect to the car, visualise sensor data and tune PID values. |
| [`Documentation`](Documentation) | Doxygen generated reference documentation.  Open `html/index.html` for a web view. |
| [`Imagini`](Imagini) | Image assets used in the documentation and README. |

## Getting Started

### ESP32 Firmware
1. Install [ESP‑IDF](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/).
2. Configure and build:
   ```bash
   cd Car-Control/Car_Controll
   idf.py set-target esp32
   idf.py build flash monitor
   ```

### Android Application
1. Ensure a JDK and Android SDK are installed.
2. Build the app from the `HappySilicon` directory:
   ```bash
   cd HappySilicon
   ./gradlew assembleDebug
   ```
   The project can also be opened directly in Android Studio.

## Usage

1. Flash the firmware to an ESP32 and power the car.  
2. Connect the Android device to the same Wi‑Fi network as the ESP32.  
3. Launch the **HappySilicon** app and use the *Connect* button to establish a UDP link.  
4. Choose **Diagnosis Mode** for manual control or **Autonomous Mode** for self‑driving.  PID coefficients can be changed from the *PID Settings* screen.

## Documentation
Generated documentation is available in the [`Documentation/html`](Documentation/html) directory.  Open `index.html` in a browser for an overview of the firmware APIs.

## License

This project is released under the [MIT License](LICENSE).

## Contributing

Pull requests and suggestions are welcome.  Please open an issue to discuss major changes before submitting a PR.

