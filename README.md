# Home Security and Safety Monitoring System (HSSMS)

This firmware implements a multi-modal security and safety system using an **ESP32** microcontroller. The system uses an ultrasonic sensor to classify human intent and environmental sensors to monitor for fire or gas hazards.

## Hardware Pin Mapping

| Component | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **Ultrasonic Trig** | GPIO 5 | Trigger sonic pulse |
| **Ultrasonic Echo** | GPIO 18 | Measure reflection time |
| **DHT11 Data** | GPIO 4 | Temp/Humidity data |
| **Gas Sensor (AO)** | GPIO 34 | Analog air quality value |
| **LED 1** | GPIO 2 | Warning indicator |
| **LED 2** | GPIO 15 | Threat indicator |
| **Buzzer** | GPIO 13 | Audible alarm |

## Security State Machine

The system analyzes a distance trajectory buffer ($N=10$) to determine intent:

* **IDLE**: No object detected within 100 cm.
* **PASSING BY**: Distance drops then increases; no alert triggered.
* **APPROACHING**: Consistent monotonic decrease in distance. LED 1 blinks at **2 Hz**.
* **STANDING**: Object stationary ($< 3$ cm variance) within 150 cm range. LED 2 solid ON, LED 1 blinks at **1 Hz**.

## Safety & Alarm Logic

* **DHT11 Duty Cycle**: To save energy, the sensor is sampled only once every **60 seconds**.
* **Gas Monitoring**: Continuous monitoring with a threshold of **600 ppm**.
* **Combined Threat**: If the state is **STANDING** while a Gas Alarm or High Temperature ($> 40$°C) is active, the buzzer emits a **rapid-beep** ($100$ ms intervals) and both LEDs remain **ON**.
* **Standard Gas Alarm**: Emits a steady **1500 Hz** tone.

## Technical Implementation

* **Sampling Rate**: Main loop runs every **200 ms**.
* **Non-Blocking Logic**: Uses `millis()` for sensor duty cycles and LED blinking to ensure the security state machine remains responsive.
* **Sound Generation**: Uses the `tone()` function for passive buzzer support.
