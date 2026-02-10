# Power System Monitoring and Safety Device - README

## Overview

This is a **Smart Energy Monitoring and Safety System** designed for residential areas in Nigeria. The system monitors power parameters (Voltage, Current, Power), displays data locally on an OLED screen, transmits data to the cloud via Blynk, and includes automatic safety protection that disconnects power during fault conditions.

## Features

✅ **Real-time Monitoring**: Continuous voltage, current, and power measurement  
✅ **Cloud Dashboard**: Blynk IoT integration for remote monitoring  
✅ **Cloud Logging**: Automatic logging to Google Sheets for historical data analysis  
✅ **Local Display**: 1.3" OLED screen showing live data  
✅ **Safety Protection**: Automatic relay trip on over-voltage, under-voltage, or over-current  
✅ **Offline Operation**: Continues monitoring and protection even without WiFi  
✅ **Battery Monitoring**: Track system uptime and battery health via heartbeat logging  
✅ **Visual Status**: RGB LED indicates system state  
✅ **Modular Architecture**: Clean C++ design with separated concerns  
✅ **Non-blocking**: Uses millis() timers, no delay() calls

## Hardware Components

- **Microcontroller**: ESP32 Dev Kit
- **Voltage Sensor**: ZMPT101b (simulated with potentiometer for testing)
- **Current Sensor**: SCT-013 100A CT (simulated with potentiometer for testing)
- **Relay**: 1-Channel Relay Module
- **Display**: 1.3" OLED (SSD1306, I2C)
- **Status LED**: RGB LED (Common Cathode)
- **Power Control**: Physical Boat Switch (hardware power on/off)
- **Power**: AC-DC 5V with 7.4V battery backup

## Pin Configuration

| Component       | GPIO Pin | Notes                        |
| --------------- | -------- | ---------------------------- |
| Voltage Sensor  | GPIO 34  | ADC1_CH6                     |
| Current Sensor  | GPIO 35  | ADC1_CH7                     |
| Relay           | GPIO 26  | Configurable Active HIGH/LOW |
| RGB LED (Red)   | GPIO 25  | PWM capable                  |
| RGB LED (Green) | GPIO 33  | PWM capable                  |
| RGB LED (Blue)  | GPIO 32  | PWM capable                  |
| OLED SDA        | GPIO 21  | I2C Data                     |
| OLED SCL        | GPIO 22  | I2C Clock                    |

## Software Architecture

### Modules

1. **EnergySensor**: Handles sensor reading with calibration
2. **DisplayManager**: Manages OLED display screens
3. **NetworkManager**: WiFi and Blynk communication
4. **SafetyManager**: Threshold monitoring and relay control
5. **CloudLogger**: Automatic logging to Google Sheets
6. **PerformanceLogger**: CSV logging via Serial and cloud integration
7. **StateManager**: Finite state machine coordinating all modules

### State Machine

- **STATE_BOOT**: Initialization and WiFi connection
- **STATE_NORMAL**: Active monitoring with cloud logging
- **STATE_TRIP_PROTECTION**: Fault detected, power disconnected
- **STATE_OFFLINE_MODE**: WiFi lost, local monitoring continues

## Getting Started

### 1. Try in Wokwi Simulator (No Hardware Required!)

You can test the entire system in your browser using Wokwi:

1. Install [Wokwi for VS Code](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode)
2. Build the project: `pio run`
3. Press `F1` → "Wokwi: Start Simulator"
4. Adjust potentiometers to simulate voltage/current changes

See [WOKWI_SIMULATION.md](WOKWI_SIMULATION.md) for detailed testing scenarios.

### 2. Install PlatformIO

Install [PlatformIO](https://platformio.org/) IDE or CLI.

### 3. Configure Credentials

**IMPORTANT:** Create your `secrets.h` file by moving the template from the `docs` folder:

```bash
# Move the template to the include folder and rename it
copy docs/secrets.h.example include/secrets.h  # On Windows
# OR
cp docs/secrets.h.example include/secrets.h    # On Linux/Mac
```

Edit `include/secrets.h` and update your credentials:

```cpp
// WiFi Credentials
#define SECRET_WIFI_SSID      "Your_WiFi_Name"
#define SECRET_WIFI_PASSWORD  "Your_WiFi_Password"

// Blynk IoT Credentials  
#define SECRET_BLYNK_TEMPLATE_ID     "Your_Template_ID"
#define SECRET_BLYNK_TEMPLATE_NAME   "Your_Template_Name"
#define SECRET_BLYNK_AUTH_TOKEN      "Your_Blynk_Token"

// Google Sheets Logging
#define SECRET_GOOGLE_SHEETS_URL     "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec"
```

> **Note:** `secrets.h` is already in `.gitignore` and will NOT be committed to version control.

### 4. Configure Safety Thresholds

Adjust thresholds in `include/config.h`:

```cpp
#define VOLTAGE_MAX           240.0  // Over-voltage trip
#define VOLTAGE_MIN           180.0  // Under-voltage trip
#define CURRENT_MAX           20.0   // Over-current trip
```

### 5. Build and Upload

```bash
pio run --target upload
pio device monitor
```

## Blynk Setup

> **[Click here for the detailed Step-by-Step Blynk Setup Guide](BLYNK_SETUP.md)**

### Virtual Pins

- **V0**: Voltage (read-only)
- **V1**: Current (read-only)
- **V2**: Power (read-only)
- **V3**: System State (read-only)
- **V4**: Reset Button (write)

### Datastreams

Create the following datastreams in your Blynk template:

1. V0 - Voltage (0-300V)
2. V1 - Current (0-100A)
3. V2 - Power (0-10000W)
4. V3 - State (String)
5. V4 - Reset (Integer 0-1)

## Cloud Logging (Google Sheets)

The system automatically logs performance data and system events to Google Sheets when WiFi is connected.

### Setup Google Apps Script

1. **Create a Google Sheet** named "ESP32 Performance Logs"
2. **Create 4 tabs**: `SystemEvents`, `Latency`, `Accuracy`, `TripResponse`
3. **Open Script Editor**: Extensions → Apps Script
4. **Paste the script** from `docs/google_apps_script.js`
5. **Deploy as Web App**:
   - Click Deploy → New deployment
   - Type: Web app
   - Execute as: Me
   - Who has access: Anyone
   - Click Deploy
   - **Copy the deployment URL**

### Configure in ESP32

Edit `include/config.h`:

```cpp
#define GOOGLE_SHEETS_URL "https://script.google.com/macros/s/YOUR_DEPLOYMENT_URL/exec"
#define ENABLE_CLOUD_LOGGING      // Enables automatic cloud logging
#define ENABLE_PERFORMANCE_LOGGING // Enables performance metrics
```

### What Gets Logged

**SystemEvents Tab:**
- BOOT - System startup
- HEARTBEAT - Every 5 minutes (includes WiFi RSSI for battery health monitoring)
- POWER_OUTAGE - Mains voltage lost
- POWER_RESTORED - Mains voltage restored
- TRIP - Safety faults with fault reason

**Latency Tab:**
- Sensor read time
- Blynk transmit time
- Total latency (logged on every Blynk transmission)

**Accuracy Tab:**
- Voltage, Current, Power readings
- Logged every 10 seconds

**TripResponse Tab:**
- Fault detection time
- Relay trip time
- Total response time (logged when safety trips occur)

### Battery Health Monitoring

Use heartbeat data to monitor battery health:

- **Continuous heartbeats during outage** = Healthy battery
- **Gaps in heartbeats** = Battery depleted or WiFi too weak
- **Declining RSSI** = Battery voltage dropping
- **BOOT after gap** = System shut down (battery dead)

## Calibration

After hardware assembly, calibrate sensors:

1. Connect a known voltage source (measure with multimeter)
2. Adjust `VOLTAGE_SLOPE` and `VOLTAGE_INTERCEPT` in `config.h`
3. Connect a known current load (measure with clamp meter)
4. Adjust `CURRENT_SLOPE` and `CURRENT_INTERCEPT` in `config.h`
5. Verify accuracy across operating range

## Safety Features

### Automatic Protection

The system continuously monitors power parameters every 100ms. If any threshold is exceeded:

1. Relay trips immediately (power disconnected)
2. RGB LED turns RED
3. OLED displays fault reason
4. Alert sent to Blynk (if connected)

### Reset Procedure

To reset after a trip:

1. Ensure fault condition is cleared
2. Press reset button in Blynk app

The system will only reset if readings are within safe limits.

### Offline Safety

**Critical**: Safety monitoring continues even when WiFi is disconnected. The relay will trip on fault conditions regardless of network status.

## RGB LED Status

- 🔵 **Blue**: Booting/Initializing
- 🟢 **Green**: Normal operation
- 🟡 **Yellow**: Offline mode (WiFi disconnected)
- 🔴 **Red**: Trip protection active

## Troubleshooting

### Display Not Working

- Check I2C address (0x3C or 0x3D)
- Verify SDA/SCL connections
- Check serial monitor for initialization errors

### WiFi Not Connecting

- Verify credentials in `config.h`
- Check 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Monitor serial output for connection attempts

### Blynk Not Connecting

- Verify auth token
- Ensure WiFi is connected first
- Check Blynk server status

### False Trips

- Adjust hysteresis values in `config.h`
- Calibrate sensors properly
- Check for electrical noise

## Migration to Production

To switch from simulation (potentiometers) to real sensors:

1. Remove `-D SIMULATION_MODE` from `platformio.ini`
2. Add EmonLib library
3. Update `EnergySensor.cpp` with EmonLib code
4. Recalibrate with actual sensors

## Documentation

### 📚 Complete Documentation Suite

- **[API Documentation](docs/API_DOCUMENTATION.md)** - Complete API reference, configuration guide, calibration procedures, and troubleshooting
- **[Technical Challenges Report](docs/TECHNICAL_CHALLENGES.md)** - Detailed analysis of all challenges faced and solutions implemented
- **[Chapter 3: Methodology](docs/CHAPTER3_METHODOLOGY.md)** - Academic methodology documentation with diagrams and algorithms
- **[Blynk Setup Guide](BLYNK_SETUP.md)** - Step-by-step IoT platform configuration
- **[Hybrid Analysis Guide](docs/HYBRID_ANALYSIS_GUIDE.md)** - Guide for combining cloud and USB data for unified analysis
- **[Wokwi Simulation Guide](WOKWI_SIMULATION.md)** - Browser-based testing without hardware

### Quick Links

| Topic | Document | Description |
|-------|----------|-------------|
| Getting Started | [API Documentation](docs/API_DOCUMENTATION.md#getting-started) | Installation and first-time setup |
| API Reference | [API Documentation](docs/API_DOCUMENTATION.md#api-reference) | Complete class and method documentation |
| Calibration | [API Documentation](docs/API_DOCUMENTATION.md#calibration-procedures) | Sensor calibration step-by-step |
| Troubleshooting | [API Documentation](docs/API_DOCUMENTATION.md#troubleshooting) | Common issues and solutions |
| Contributing | [API Documentation](docs/API_DOCUMENTATION.md#contributing) | Code style and contribution guidelines |
| Future Plans | [API Documentation](docs/API_DOCUMENTATION.md#future-enhancements) | Roadmap and planned features |

## License

MIT License - Free for educational and commercial use

## Support

For issues or questions, please open an issue on GitHub.

---

**⚠️ Safety Warning**: This system controls mains power. Only qualified electricians should install and wire the hardware. Improper installation can result in electric shock, fire, or death.
