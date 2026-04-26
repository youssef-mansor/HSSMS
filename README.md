# Home Security and Safety Monitoring System (HSSMS)

This project implements a multi-modal security and safety system with two main components:
1. **Arduino Firmware** - ESP32-based real-time security and environmental monitoring
2. **Contiki/Cooja Simulation** - Network simulation for distributed sensor nodes

## Overview

The ESP32 firmware uses an ultrasonic sensor to classify human intent and environmental sensors to monitor for fire or gas hazards. The system includes a WiFi-enabled web interface for remote monitoring and alerts.

## Hardware Pin Mapping (ESP32)

| Component | ESP32 Pin | Function | Type |
| :--- | :--- | :--- | :--- |
| **Ultrasonic Trigger** | GPIO 5 | Trigger sonic pulse | Digital Output |
| **Ultrasonic Echo** | GPIO 18 | Measure reflection time | Digital Input |
| **DHT11 Data** | GPIO 4 | Temperature/Humidity sensor | Digital I/O |
| **Gas Sensor (Analog)** | GPIO 34 | Air quality detection | Analog Input |
| **LED 1** | GPIO 2 | Warning indicator (approaching) | Digital Output |
| **LED 2** | GPIO 15 | Threat indicator (standing/alarm) | Digital Output |
| **Buzzer** | GPIO 13 | Audible alarm (passive buzzer) | Digital Output |

## Network & Web Interface

* **WiFi SSID/Password**: Configured for local network connectivity (currently hardcoded as "Adham")
* **Web Server**: Runs on GPIO 80 using AsyncWebServer library
* **Auto-Refresh**: Dashboard automatically refreshes every 1 second
* **Monitoring Page**: Displays real-time system state, distance readings, temperature, and gas levels
* **Offline Mode**: If WiFi connection fails, the system continues operating locally without web monitoring

## Security State Machine

The system analyzes a distance trajectory buffer ($N=10$) to determine intent with the following states:

* **IDLE**: No object detected within 100 cm or distance reading exceeds 100 cm.
* **PASSING BY**: Object distance drops then increases; variance exceeds threshold (no direct threat). Both LEDs OFF.
* **APPROACHING**: Consistent monotonic decrease in distance (object moving closer). LED 1 blinks at **2 Hz** (250 ms period).
* **STANDING**: Object stationary with low variance ($\leq 3$ cm) within 150 cm range. LED 1 blinks at **1 Hz** (500 ms period), LED 2 solid ON.

### State Transition Logic

The system continuously evaluates the buffer using three detection functions:
1. **STANDING detection**: All 10 readings have variance $\leq 3$ cm AND distance $< 150$ cm
2. **APPROACHING detection**: All 10 readings show monotonic decrease (each reading $>$ next reading)
3. **PASSING BY detection**: First and last readings $>$ middle reading (drop-and-rise pattern)
4. **IDLE**: Default when distance $> 100$ cm or readings invalid

## Safety & Alarm Logic

### Environmental Monitoring
* **DHT11 Duty Cycle**: To conserve energy, temperature/humidity is sampled once every **60 seconds** (60,000 ms)
* **Gas Threshold**: Continuous analog monitoring with alert threshold set at **600 ppm** (lower reading = higher concentration due to inverted sensor logic)
* **Temperature Threshold**: High temperature alarm triggers at **> 40°C**

### Alarm Behavior
* **Critical Threat (STANDING + Gas Alarm OR High Temp)**: 
  - Buzzer emits **rapid beeping** at **2500 Hz** with **100 ms intervals**
  - Both LED 1 and LED 2 remain **solid ON**
  
* **Gas Hazard Only**: 
  - Buzzer emits steady tone at **1500 Hz**
  - LED indicators follow normal state machine (blinking or OFF based on distance state)
  
* **Normal Operation**: 
  - Buzzer OFF when no hazards detected

## Technical Implementation

### Core Software Architecture
* **Main Loop Frequency**: 200 ms interval between cycles
* **Ultrasonic Sampling**: Distance read every 200 ms with timeout protection (max 26 ms pulse wait)
* **Non-Blocking Processing**: Uses `millis()` for all timing and sensor duty cycles to maintain responsiveness
* **Buffer Management**: Circular buffer (size 10) stores distance readings with modulo indexing
* **Sound Generation**: Uses Arduino `tone()` function for passive buzzer support (compatible with ESP32)

### State Machine Implementation
* Lambda functions used for efficient state transition detection
* Standing detection: Variance calculation across entire buffer
* Approaching detection: Monotonic decrease validation across all readings
* Passing-by detection: Comparison of first, middle, and last buffer values

### Libraries Used
* `WiFi.h` - ESP32 WiFi connectivity
* `ESPAsyncWebServer.h` - Asynchronous HTTP server for web dashboard
* `DHT.h` - DHT11 sensor protocol

### Serial Communication
* **Baud Rate**: 115200
* **Output**: Minimal serial logging to prevent glitching (sample output every 1000 ms)
* **Bootup Info**: WiFi connection attempts and IP address reported on startup

## Cooja Simulation

The `cooja/` directory contains a Contiki-based network simulation for testing distributed sensor nodes without physical hardware.

### Files Overview

| File | Purpose |
| :--- | :--- |
| `sensor.c` | Sensor node implementation that broadcasts simulated readings |
| `gateway.c` | Gateway node for receiving and aggregating sensor data |
| `HSSMS.csc` | Cooja simulation configuration file (network topology, nodes, plugins) |
| `Makefile` | Build configuration for Contiki compilation |
| `symbols.c / symbols.h` | Symbol table for debugging and monitoring |
| `*.sky` | Sky platform target binaries |
| `obj_sky/` | Build artifacts and dependency files |

### Sensor Simulation

The sensor nodes simulate HSSMS readings every **5 seconds** using broadcast communication:

* **Distance**: Randomly simulated 0-200 cm
* **Temperature**: Randomly simulated 20-35°C  
* **Gas Level**: Randomly simulated 300-500 ppm

#### Message Format
```
D:<distance>, T:<temperature>, G:<gas_value>
```

Example: `D:142, T:28, G:431`

### Network Communication

* **Protocol**: Broadcast over Rime network stack (broadcast channel 129)
* **Broadcast Channel**: 129
* **Interval**: 5 seconds between transmissions
* **Payload**: Null-terminated ASCII string with sensor readings

### Running the Simulation

1. **Prerequisites**: Contiki OS environment and Cooja installed
2. **Build**: Navigate to `cooja/` directory and run:
   ```bash
   make
   ```
3. **Simulate**: Open `HSSMS.csc` in Cooja to start the simulation
4. **Monitor**: Use Cooja's serial output window to observe sensor broadcasts
5. **Customize**: Modify sensor ranges and timing in `sensor.c` and `gateway.c` as needed

### Extending the Simulation

* **Add More Sensors**: Duplicate sensor nodes in the `.csc` configuration
* **Modify Timings**: Change `CLOCK_SECOND * 5` to adjust broadcast interval
* **Update Thresholds**: Modify the simulated value ranges in the sensor code
* **Add Gateway Logic**: Implement state machine evaluation in `gateway.c`
