# Smart Energy Monitoring & Safety System - Complete Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [System Architecture](#system-architecture)
3. [Getting Started](#getting-started)
4. [API Reference](#api-reference)
5. [Configuration Guide](#configuration-guide)
6. [Calibration Procedures](#calibration-procedures)
7. [Troubleshooting](#troubleshooting)
8. [Contributing](#contributing)
9. [Future Enhancements](#future-enhancements)
10. [License](#license)

---

## Project Overview

### Purpose
A real-time energy monitoring system with intelligent safety protection for AC electrical loads. The system monitors voltage and current, calculates power consumption, and automatically disconnects the load when dangerous conditions are detected.

### Key Features
- ✅ Real-time voltage and current monitoring
- ✅ Automatic over/under voltage protection
- ✅ Over-current protection with configurable thresholds
- ✅ OLED display for local monitoring
- ✅ IoT connectivity via Blynk platform
- ✅ RGB LED status indicators
- ✅ Smart power-off detection (no false trips)
- ✅ Exponential moving average filtering for stability
- ✅ Hysteresis to prevent relay chattering

### Hardware Requirements
- **Microcontroller**: ESP32 Development Board
- **Voltage Sensor**: ZMPT101B AC Voltage Sensor
- **Current Sensor**: SCT-013 Current Transformer (30A max)
- **Display**: SSD1306 128x64 OLED (I2C)
- **Relay**: 1-Channel 5V Relay Module (NO contact)
- **Status LED**: RGB LED (Common Cathode)
- **Power Supply**: 5V DC adapter

### Software Stack
- **Framework**: Arduino (PlatformIO)
- **Platform**: Espressif ESP32
- **Libraries**:
  - `Blynk` (v1.3.2) - IoT connectivity
  - `U8g2` (v2.36.15) - OLED display driver
  - `WiFi` - Built-in ESP32 WiFi
  - `Wire` - I2C communication

---

## System Architecture

### Module Overview

```
┌─────────────────────────────────────────────────────────┐
│                     Main Application                     │
│                      (main.cpp)                          │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│                    State Manager                         │
│           (Finite State Machine Coordinator)             │
└─┬───────┬───────┬───────┬───────────────────────────────┘
  │       │       │       │
  ▼       ▼       ▼       ▼
┌────┐ ┌────┐ ┌────┐ ┌─────────┐
│Sensor│Display│Safety│ Network │
│Module│Module │Module│ Module  │
└────┘ └────┘ └────┘ └─────────┘
  │       │       │       │
  ▼       ▼       ▼       ▼
┌────────────────────────────────┐
│      Hardware Abstraction      │
│   (ADC, I2C, GPIO, WiFi)       │
└────────────────────────────────┘
```

### State Machine

The system operates in four distinct states:

1. **BOOT**: Initialization and WiFi connection
2. **NORMAL**: Active monitoring with cloud connectivity
3. **OFFLINE_MODE**: Local monitoring (WiFi unavailable)
4. **TRIP_PROTECTION**: Fault detected, relay disconnected

### Data Flow

```
Sensors → ADC → RMS Calculation → Smoothing → Safety Check → Relay Control
                                      ↓
                                  Display Update
                                      ↓
                                  Cloud Publish
```

---

## Getting Started

### Prerequisites

1. **Install PlatformIO**
   ```bash
   # Using VS Code
   # Install PlatformIO IDE extension from marketplace
   
   # Or using CLI
   pip install platformio
   ```

2. **Clone Repository**
   ```bash
   git clone https://github.com/yourusername/Samuel-Project.git
   cd Samuel-Project
   ```

3. **Configure Credentials**
   
   Create `include/secret.h`:
   ```cpp
   #ifndef SECRET_H
   #define SECRET_H
   
   #define SECRET_WIFI_SSID "YourWiFiSSID"
   #define SECRET_WIFI_PASSWORD "YourWiFiPassword"
   
   #endif
   ```

4. **Update Blynk Configuration**
   
   In `include/config.h`, update:
   ```cpp
   #define BLYNK_TEMPLATE_ID "YourTemplateID"
   #define BLYNK_AUTH_TOKEN "YourAuthToken"
   ```

### Building and Uploading

```bash
# Build project
pio run

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor
```

### First-Time Setup

1. **Power on the system**
2. **Wait for WiFi connection** (blue LED indicates connecting)
3. **Verify display shows startup screen**
4. **Check serial monitor for initialization logs**
5. **Proceed to calibration** (see Calibration section)

---

## API Reference

### EnergySensor Class

**Purpose**: Handles voltage and current sensing with RMS calculation and filtering.

#### Public Methods

```cpp
void begin()
```
- **Description**: Initialize ADC and sensor configuration
- **Parameters**: None
- **Returns**: void
- **Usage**:
  ```cpp
  EnergySensor sensor;
  sensor.begin();
  ```

```cpp
void update()
```
- **Description**: Read sensors, calculate RMS, apply smoothing
- **Call Frequency**: Every 500ms recommended
- **Side Effects**: Updates internal voltage_ and current_ values

```cpp
float getVoltage() const
```
- **Returns**: Smoothed voltage in Volts (V)
- **Range**: 0.0 - 300.0V
- **Accuracy**: ±2% after calibration

```cpp
float getCurrent() const
```
- **Returns**: Smoothed current in Amperes (A)
- **Range**: 0.0 - 30.0A
- **Accuracy**: ±5% after calibration

```cpp
float getPower() const
```
- **Returns**: Calculated power (V × I) in Watts (W)
- **Note**: Assumes unity power factor

```cpp
bool isValid() const
```
- **Returns**: true if readings are within reasonable bounds
- **Use Case**: Validate sensor connectivity

#### Private Methods

```cpp
float readVoltageSimulation()
```
- **Algorithm**: Single-pass RMS with dynamic DC bias removal
- **Samples**: 1000 samples over ~100ms
- **Formula**: `RMS = sqrt((ΣV² - (ΣV)²/N) / N)`

```cpp
float readCurrentSimulation()
```
- **Algorithm**: Two-pass RMS (DC bias then RMS)
- **Samples**: 500 samples over ~100ms

---

### SafetyManager Class

**Purpose**: Monitors electrical parameters and controls relay for protection.

#### Public Methods

```cpp
void begin()
```
- **Description**: Initialize relay and RGB LED pins
- **Initial State**: Relay OFF, RGB Blue

```cpp
bool checkSafety(float voltage, float current)
```
- **Parameters**:
  - `voltage`: Current voltage reading (V)
  - `current`: Current current reading (A)
- **Returns**: `true` if safe, `false` if fault detected
- **Side Effects**: Sets `lastFaultReason_` on fault
- **Logic**:
  ```
  IF voltage < 100V THEN return true (power off)
  IF voltage > 250V THEN return false (over-voltage)
  IF voltage < 216V THEN return false (under-voltage)
  IF current > 30A THEN return false (over-current)
  ELSE return true
  ```

```cpp
void tripRelay()
```
- **Description**: De-energize relay (disconnect load)
- **Visual**: Sets RGB to RED
- **State**: Sets `relayTripped_ = true`

```cpp
void resetRelay()
```
- **Description**: Energize relay (connect load)
- **Precondition**: Safety conditions must be met
- **State**: Sets `relayTripped_ = false`

```cpp
bool isTripped() const
```
- **Returns**: Current relay state
- **Use Case**: Check if system is in fault state

```cpp
String getLastFaultReason() const
```
- **Returns**: "OVER VOLTAGE", "UNDER VOLTAGE", or "OVER CURRENT"
- **Use Case**: Display fault reason to user

```cpp
void setRGBStatus(uint8_t r, uint8_t g, uint8_t b)
```
- **Parameters**: RGB values (0-255)
- **Note**: Automatically inverts for common anode LEDs
- **Predefined Colors**:
  - `RGB_BLUE`: Booting/Offline
  - `RGB_GREEN`: Normal operation
  - `RGB_RED`: Fault/Trip
  - `RGB_YELLOW`: Warning (unused)

---

### DisplayManager Class

**Purpose**: Manages OLED display output for various system states.

#### Public Methods

```cpp
bool begin()
```
- **Returns**: `true` if display initialized successfully
- **Display**: SSD1306 128x64 I2C
- **Address**: 0x3C

```cpp
void showStartup()
```
- **Display**: Logo and loading bar
- **Duration**: Shown during BOOT state (~3s)

```cpp
void showData(float voltage, float current, float power, 
              bool wifiConnected, bool blynkConnected)
```
- **Layout**:
  ```
  ┌────────────────────────┐
  │ ACTIVE [WiFi][Blynk]   │
  ├────────────────────────┤
  │ VOLT                   │
  │      223.5 V           │
  ├────────────────────────┤
  │ CURR     │  PWR        │
  │ 12.5 A   │  2.8 kW     │
  └────────────────────────┘
  ```

```cpp
void showTripAlert(const String& reason)
```
- **Display**: Inverted screen with "TRIPPED!" and fault reason
- **Visual**: High contrast for attention

```cpp
void showOfflineMode()
```
- **Display**: "OFFLINE MODE" with local sensor data
- **Use Case**: WiFi disconnected

```cpp
void clear()
```
- **Description**: Clear display buffer and screen

---

### StateManager Class

**Purpose**: Coordinates all modules using finite state machine.

#### Public Methods

```cpp
void begin()
```
- **Description**: Initialize state machine
- **Initial State**: BOOT

```cpp
void update()
```
- **Description**: Execute current state logic
- **Call Frequency**: Every loop iteration
- **Non-blocking**: Uses millis() for timing

```cpp
void setState(SystemState newState)
```
- **Parameters**: `STATE_BOOT`, `STATE_NORMAL`, `STATE_OFFLINE_MODE`, `STATE_TRIP_PROTECTION`
- **Side Effects**: Calls `onStateExit()` and `onStateEnter()`

```cpp
SystemState getState() const
```
- **Returns**: Current system state

```cpp
String getStateName() const
```
- **Returns**: Human-readable state name

```cpp
void handleReset()
```
- **Description**: Handle manual reset from button or Blynk
- **Precondition**: Must be in TRIP_PROTECTION state
- **Safety Check**: Verifies conditions are safe before reset

#### State Update Methods (Private)

```cpp
void updateStateBoot()
```
- **Timing**: 3s startup delay
- **Transition**: → NORMAL (WiFi OK) or OFFLINE_MODE (timeout)

```cpp
void updateStateNormal()
```
- **Tasks**:
  - Read sensors (500ms interval)
  - Check safety (100ms interval)
  - Update display (1000ms interval)
  - Publish to Blynk (2000ms interval)
- **Transitions**:
  - → TRIP_PROTECTION (fault detected)
  - → OFFLINE_MODE (WiFi lost)

```cpp
void updateStateTripProtection()
```
- **Tasks**:
  - Continue sensor reading
  - Display fault alert
  - Publish fault to cloud
- **Transition**: → NORMAL/OFFLINE (manual reset + safe)

```cpp
void updateStateOfflineMode()
```
- **Tasks**:
  - Local monitoring continues
  - Attempt WiFi reconnection
  - Safety checks active
- **Transitions**:
  - → NORMAL (WiFi restored)
  - → TRIP_PROTECTION (fault detected)

---

### NetworkManager Class

**Purpose**: Handle WiFi and Blynk cloud connectivity.

#### Public Methods

```cpp
void begin()
```
- **Description**: Start WiFi connection
- **Non-blocking**: Connection happens in background

```cpp
void update()
```
- **Description**: Maintain WiFi and Blynk connections
- **Call Frequency**: Every loop iteration
- **Tasks**:
  - Monitor WiFi status
  - Run Blynk.run()
  - Attempt reconnection if needed

```cpp
void publishData(float voltage, float current, float power)
```
- **Description**: Send sensor data to Blynk
- **Throttling**: Internal 2s minimum interval
- **Virtual Pins**:
  - V0: Voltage
  - V1: Current
  - V2: Power

```cpp
void sendAlert(const String& message)
```
- **Description**: Send push notification via Blynk
- **Event**: "safety_alert"
- **Use Case**: Fault detection

```cpp
void updateState(const String& state)
```
- **Description**: Update system state on cloud
- **Virtual Pin**: V3

```cpp
bool isWiFiConnected() const
```
- **Returns**: WiFi connection status

```cpp
bool isBlynkConnected() const
```
- **Returns**: Blynk cloud connection status

```cpp
void setResetCallback(void (*callback)())
```
- **Description**: Register callback for Blynk reset button
- **Virtual Pin**: V4 (write)

---

## Configuration Guide

### config.h Reference

#### Pin Definitions

```cpp
// Analog Sensors (ADC1 only - ADC2 conflicts with WiFi)
#define PIN_VOLTAGE_SENSOR    34  // GPIO 34
#define PIN_CURRENT_SENSOR    35  // GPIO 35

// Digital Outputs
#define PIN_RELAY             26  // GPIO 26
#define PIN_RGB_RED           25  // GPIO 25
#define PIN_RGB_GREEN         33  // GPIO 33
#define PIN_RGB_BLUE          32  // GPIO 32

// I2C (OLED Display)
#define PIN_SDA               21  // GPIO 21
#define PIN_SCL               22  // GPIO 22
```

**⚠️ Important**: Do not use ADC2 pins (GPIO 0, 2, 4, 12-15, 25-27) for analog input when WiFi is active.

#### Safety Thresholds

```cpp
// Voltage Limits
#define VOLTAGE_MAX           250.0  // Over-voltage trip (V)
#define VOLTAGE_MIN           216.0  // Under-voltage trip (V)
#define VOLTAGE_HYSTERESIS    5.0    // Prevent chattering (V)
#define VOLTAGE_POWER_PRESENT_THRESHOLD 100.0  // Power detection (V)

// Current Limits
#define CURRENT_MAX           30.0   // Over-current trip (A)
#define CURRENT_HYSTERESIS    0.5    // Prevent chattering (A)
```

**Customization Guide**:
- **VOLTAGE_MAX**: Set to 110% of nominal voltage (e.g., 230V × 1.1 = 253V)
- **VOLTAGE_MIN**: Set to 90% of nominal voltage (e.g., 230V × 0.9 = 207V)
- **CURRENT_MAX**: Set to rated current of protected load
- **HYSTERESIS**: 2-5V for voltage, 0.5-1A for current

#### Calibration Constants

```cpp
// Voltage Sensor (ZMPT101B)
#define VOLTAGE_CALIBRATION_FACTOR  556.0  // Adjust based on calibration
#define VOLTAGE_NOISE_THRESHOLD     75.0   // Ghost voltage filter (V)

// Current Sensor (SCT-013)
#define CURRENT_NOISE_THRESHOLD     0.60   // Ghost current filter (A)
```

**Calibration Formula**:
```
Calibration Factor = Actual Value (multimeter) / Raw RMS (serial log)
```

#### Timing Intervals

```cpp
#define INTERVAL_SENSOR_READ  500    // Sensor reading (ms)
#define INTERVAL_DISPLAY      1000   // Display update (ms)
#define INTERVAL_BLYNK        2000   // Blynk publish (ms)
#define INTERVAL_SAFETY       100    // Safety check (ms) - CRITICAL
#define INTERVAL_WIFI_RETRY   30000  // WiFi reconnect (ms)
```

**Performance Tuning**:
- Decrease `INTERVAL_SAFETY` for faster fault detection (min: 50ms)
- Increase `INTERVAL_BLYNK` to reduce cloud traffic
- Adjust `INTERVAL_SENSOR_READ` based on load dynamics

---

## Calibration Procedures

### Voltage Sensor Calibration

**Required Equipment**:
- Calibrated multimeter (True RMS)
- Stable AC power source

**Procedure**:

1. **Enable Debug Logging**
   - Ensure `#define APP_DEBUG` is uncommented in code
   - Upload firmware and open serial monitor (115200 baud)

2. **Measure Reference Voltage**
   ```
   Connect multimeter to AC source
   Record reading (e.g., 228.5V)
   ```

3. **Read Raw RMS**
   ```
   Look for log line:
   [EnergySensor] Voltage RMS: 0.4105V -> 228.27V AC
                                ^^^^^^
                                This is Raw RMS
   ```

4. **Calculate Calibration Factor**
   ```
   Factor = Actual Voltage / Raw RMS
   Factor = 228.5 / 0.4105 = 556.6
   ```

5. **Update Configuration**
   ```cpp
   // In config.h
   #define VOLTAGE_CALIBRATION_FACTOR  556.6
   ```

6. **Verify**
   ```
   Re-upload firmware
   Check that displayed voltage matches multimeter
   Tolerance: ±2%
   ```

### Current Sensor Calibration

**Required Equipment**:
- Calibrated clamp meter or current meter
- Known resistive load (e.g., heater, incandescent bulb)

**Procedure**:

1. **Connect Known Load**
   ```
   Use purely resistive load (no motors/transformers)
   Measure actual current with clamp meter
   Record reading (e.g., 5.2A)
   ```

2. **Read Raw RMS**
   ```
   Check serial monitor for current RMS value
   (May need to add debug logging similar to voltage)
   ```

3. **Calculate Factor**
   ```
   Factor = Actual Current / Raw RMS
   ```

4. **Update Code**
   ```cpp
   // In EnergySensor.cpp, readCurrentSimulation()
   float calibrationFactor = 20.0;  // Update this value
   ```

5. **Test Multiple Loads**
   ```
   Verify accuracy at:
   - Low current (1-5A)
   - Medium current (5-15A)
   - High current (15-30A)
   ```

### Threshold Calibration

**Noise Floor Measurement**:

1. **Power OFF Test**
   ```
   Disconnect AC input
   Record voltage/current readings
   Should be 0.0V / 0.0A
   If not, increase NOISE_THRESHOLD
   ```

2. **Power ON Test**
   ```
   Connect normal AC
   Monitor for 5 minutes
   Record min/max voltage fluctuations
   Set VOLTAGE_MIN/MAX with margin
   ```

3. **Trip Test**
   ```
   Manually trigger each condition:
   - Over-voltage: Use variable transformer
   - Under-voltage: Reduce input voltage
   - Over-current: Connect high load
   
   Verify relay trips correctly
   Verify reset works when safe
   ```

---

## Troubleshooting

### Common Issues

#### 1. Voltage Reads 0V When Power is ON

**Symptoms**:
```
[EnergySensor] Voltage RMS: 0.0441V -> 0.00V AC
```

**Causes**:
- ZMPT101B gain too low
- Wiring issue
- Sensor not receiving AC input

**Solutions**:
1. **Check Wiring**
   ```
   ZMPT101B OUT → ESP32 GPIO 34
   ZMPT101B VCC → 5V
   ZMPT101B GND → GND
   AC Wire → ZMPT101B Input Terminals
   ```

2. **Adjust Gain**
   ```
   Turn blue potentiometer CLOCKWISE
   Monitor serial output
   Target: Raw RMS = 0.4-0.5V
   ```

3. **Verify AC Input**
   ```
   Use multimeter on ZMPT101B input
   Should read mains voltage (e.g., 230V)
   ```

#### 2. System Trips When Power is Turned OFF

**Symptoms**:
- Relay trips immediately when AC disconnected
- State changes to TRIP_PROTECTION

**Cause**:
- EMA smoothing causing slow voltage decay
- Power-present detection not working

**Solution**:
```cpp
// Verify in EnergySensor.cpp
if (rawVoltage < VOLTAGE_NOISE_THRESHOLD) {
    voltage_ = 0.0;  // Should reset immediately
}

// Verify in SafetyManager.cpp
if (voltage < VOLTAGE_POWER_PRESENT_THRESHOLD) {
    return true;  // Should skip voltage checks
}
```

#### 3. WiFi Won't Connect

**Symptoms**:
```
[NetworkManager] WiFi disconnected
System stuck in BOOT state
```

**Solutions**:
1. **Check Credentials**
   ```cpp
   // In secret.h
   #define SECRET_WIFI_SSID "CorrectSSID"  // Case-sensitive!
   #define SECRET_WIFI_PASSWORD "CorrectPassword"
   ```

2. **Check WiFi Band**
   ```
   ESP32 only supports 2.4GHz
   Ensure router has 2.4GHz enabled
   ```

3. **Increase Timeout**
   ```cpp
   // In config.h
   #define WIFI_CONNECT_TIMEOUT  30000  // Increase to 30s
   ```

#### 4. Display Not Working

**Symptoms**:
- Blank screen
- Initialization failed message

**Solutions**:
1. **Check I2C Address**
   ```cpp
   // In config.h
   #define SCREEN_ADDRESS 0x3C  // Try 0x3D if 0x3C fails
   ```

2. **Scan I2C Bus**
   ```cpp
   // Add to setup()
   Wire.begin(PIN_SDA, PIN_SCL);
   for (byte i = 0; i < 127; i++) {
       Wire.beginTransmission(i);
       if (Wire.endTransmission() == 0) {
           Serial.printf("Found device at 0x%02X\n", i);
       }
   }
   ```

3. **Check Wiring**
   ```
   SDA → GPIO 21
   SCL → GPIO 22
   VCC → 3.3V (not 5V!)
   GND → GND
   ```

#### 5. Relay Chattering

**Symptoms**:
- Relay clicks rapidly on/off
- Voltage near threshold

**Cause**:
- Insufficient hysteresis
- Noisy voltage readings

**Solutions**:
1. **Increase Hysteresis**
   ```cpp
   #define VOLTAGE_HYSTERESIS 10.0  // Increase from 5.0
   ```

2. **Increase Smoothing**
   ```cpp
   // In EnergySensor.cpp
   const float alpha = 0.1;  // Decrease from 0.2 (more smoothing)
   ```

### Debug Logging

**Enable Verbose Logging**:
```cpp
// In config.h
#define APP_DEBUG  // Uncomment this line

// For even more detail
#define DEBUG_SERIAL  // Uncomment for sensor-level logs
```

**Serial Monitor Settings**:
- Baud Rate: 115200
- Line Ending: Both NL & CR
- Filter: None

**Key Log Messages**:
```
[Main] Initialization complete
[StateManager] State transition: BOOT -> NORMAL
[EnergySensor] Voltage RMS: 0.4105V -> 228.27V AC
[SafetyManager] FAULT: Under-voltage detected (215.30V)
[NetworkManager] WiFi connected!
[NetworkManager] Published to Blynk: V=228.50, I=12.30, P=2810.95
```

---

## Contributing

### Code Style Guidelines

**Naming Conventions**:
```cpp
// Classes: PascalCase
class EnergySensor { };

// Methods: camelCase
void updateSensor();

// Private members: camelCase with trailing underscore
float voltage_;

// Constants: UPPER_SNAKE_CASE
#define VOLTAGE_MAX 250.0

// Local variables: camelCase
float currentValue;
```

**File Organization**:
```
include/
  ├── ModuleName.h      // Class declaration
  └── config.h          // Configuration constants

src/
  ├── ModuleName.cpp    // Class implementation
  └── main.cpp          // Application entry point
```

**Documentation**:
```cpp
/**
 * @brief Brief description of function
 * 
 * Detailed description if needed.
 * 
 * @param paramName Description of parameter
 * @return Description of return value
 * 
 * @note Important notes
 * @warning Warnings about usage
 */
void functionName(int paramName);
```

### Adding New Features

**1. Create Feature Branch**
```bash
git checkout -b feature/new-feature-name
```

**2. Implement Feature**
- Follow existing module structure
- Add unit tests if applicable
- Update documentation

**3. Test Thoroughly**
- Unit tests
- Integration tests
- Hardware validation

**4. Submit Pull Request**
- Clear description
- Reference any issues
- Include test results

### Module Extension Example

**Adding a New Sensor**:

1. **Create Header** (`include/NewSensor.h`):
```cpp
#ifndef NEW_SENSOR_H
#define NEW_SENSOR_H

class NewSensor {
public:
    void begin();
    void update();
    float getValue() const;
    
private:
    float value_;
    float readSensor();
};

#endif
```

2. **Implement** (`src/NewSensor.cpp`):
```cpp
#include "NewSensor.h"
#include "config.h"

void NewSensor::begin() {
    // Initialize sensor
}

void NewSensor::update() {
    value_ = readSensor();
}

float NewSensor::getValue() const {
    return value_;
}

float NewSensor::readSensor() {
    // Sensor reading logic
    return 0.0;
}
```

3. **Integrate** (`src/main.cpp`):
```cpp
#include "NewSensor.h"

NewSensor newSensor;

void setup() {
    newSensor.begin();
}

void loop() {
    newSensor.update();
    float value = newSensor.getValue();
}
```

---

## Future Enhancements

### Planned Features

#### Phase 1: Core Improvements
- [ ] **SD Card Logging**
  - Store historical data locally
  - CSV export for analysis
  - Circular buffer for memory efficiency

- [ ] **Web Server Interface**
  - Local web dashboard (ESP32 as AP)
  - Real-time graphs
  - Configuration via web UI

- [ ] **MQTT Support**
  - Alternative to Blynk
  - Home Assistant integration
  - Custom broker support

#### Phase 2: Advanced Features
- [ ] **Energy Billing**
  - kWh accumulation
  - Cost calculation
  - Monthly reports

- [ ] **Load Scheduling**
  - Time-based relay control
  - Peak/off-peak optimization
  - Programmable schedules

- [ ] **Predictive Maintenance**
  - Trend analysis
  - Anomaly detection
  - Fault prediction

#### Phase 3: Hardware Expansion
- [ ] **Multi-Channel Support**
  - Monitor multiple circuits
  - Individual protection per channel
  - Load balancing

- [ ] **Power Quality Analysis**
  - Harmonic analysis
  - Power factor measurement
  - Frequency monitoring

- [ ] **Battery Backup**
  - UPS functionality
  - Seamless switchover
  - Battery health monitoring

### Scalability Considerations

**Horizontal Scaling** (Multiple Units):
```cpp
// Assign unique device IDs
#define DEVICE_ID "UNIT_001"

// MQTT topic structure
String topic = "energy/" + String(DEVICE_ID) + "/voltage";
```

**Vertical Scaling** (More Features):
```cpp
// Modular architecture allows easy addition
class PowerQualityAnalyzer {
    // New module for advanced features
};
```

**Performance Optimization**:
- Use FreeRTOS tasks for parallel processing
- Implement DMA for ADC sampling
- Optimize display updates (dirty regions)

### Architecture Evolution

**Current**: Monolithic state machine
```
StateManager → All Modules
```

**Future**: Event-driven architecture
```
Event Bus ← Modules publish events
    ↓
Subscribers react to events
```

**Benefits**:
- Loose coupling
- Easier testing
- Plugin architecture

---

## License

This project is licensed under the MIT License - see LICENSE file for details.

---

## Support

### Documentation
- [Technical Challenges Report](docs/TECHNICAL_CHALLENGES.md)
- [Chapter 3: Methodology](docs/CHAPTER3_METHODOLOGY.md)
- [Blynk Setup Guide](BLYNK_SETUP.md)
- [Wokwi Simulation](WOKWI_SIMULATION.md)

### Contact
- **Author**: Samuel
- **Email**: [your-email@example.com]
- **GitHub**: [https://github.com/yourusername/Samuel-Project]

### Acknowledgments
- ESP32 Community
- PlatformIO Team
- Blynk Platform
- U8g2 Library Contributors

---

**Last Updated**: December 2025
**Version**: 1.0.0
**Status**: Production Ready
