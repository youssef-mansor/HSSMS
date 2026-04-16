# HSSMS Project: Part 1 - Security Subsystem

## Project Overview
Design and implementation of a Home Security and Safety Monitoring System (HSSMS) using an ESP32 microcontroller. This module focuses on the proximity-based intent detection state machine.

## Hardware Configuration
| Component | ESP32 Pin | Connection Type |
| :--- | :--- | :--- |
| **HC-SR04 Trig** | GPIO 5 | Output |
| **HC-SR04 Echo** | GPIO 18 | Input |
| **LED 1 (Warning)**| GPIO 2 | Output (PWM/Digital) |
| **LED 2 (Threat)** | GPIO 15 | Output (Digital) |
| **VCC (Sensor)** | VIN / 5V | Power |
| **GND** | GND | Ground |

## State Machine Logic
The system maintains a distance trajectory buffer of size $N=10$ to classify motion intent:

* **IDLE**: No object within 100 cm. Outputs: All OFF.
* **PASSING BY**: Detected V-shaped trajectory (distance decreases then increases). Outputs: All OFF.
* **APPROACHING**: Sustained monotonic decrease in distance across the buffer. Outputs: LED 1 blinks at 2 Hz.
* **STANDING**: Stable readings (variance < 3cm) within 70 cm range. Outputs: LED 2 solid ON, LED 1 blinks at 1 Hz.



## Code Implementation Summary
* **Sampling**: Ultrasonic pulses are triggered every 200ms via `readDist()`.
* **Data Structure**: A circular array `buffer[N]` stores the proximity history.
* **Evaluation**: Sequential checks for stability (`checkStable`), monotonicity (`checkMonotonic`), and passing patterns (`checkPassing`) determine the state.
* **Feedback**: Non-blocking `millis()` timing controls LED blink frequencies.
