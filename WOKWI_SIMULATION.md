# Wokwi Simulation Guide

## Overview

This project includes a complete Wokwi simulation that allows you to test the Smart Energy Monitoring System in your browser without physical hardware!

## Simulation Components

The simulation includes:
- ✅ **ESP32 DevKit V1**
- ✅ **SSD1306 OLED Display** (128x64, I2C)
- ✅ **2x Potentiometers** (simulating voltage and current sensors)
- ✅ **Relay Module** (1-channel)
- ✅ **RGB LED** (Common Cathode with 220Ω resistors)

## How to Run the Simulation

### Option 1: Wokwi for VS Code Extension

1. **Install Extension**:
   - Open VS Code
   - Install "Wokwi Simulator" extension
   - Restart VS Code

2. **Build Firmware**:
   ```bash
   # In VS Code terminal
   pio run
   ```

3. **Start Simulation**:
   - Press `F1` or `Ctrl+Shift+P`
   - Type "Wokwi: Start Simulator"
   - Select the project

4. **Interact**:
   - Adjust potentiometers to simulate voltage/current changes
   - Watch OLED display update
   - Observe RGB LED color changes
   - Monitor serial output

### Option 2: Wokwi CLI

1. **Install Wokwi CLI**:
   ```bash
   npm install -g @wokwi/cli
   ```

2. **Build Firmware**:
   ```bash
   pio run
   ```

3. **Run Simulation**:
   ```bash
   wokwi-cli --timeout 0
   ```

### Option 3: Wokwi Web (Manual Upload)

1. Go to [https://wokwi.com/](https://wokwi.com/)
2. Create new ESP32 project
3. Copy `diagram.json` content to the diagram editor
4. Upload your compiled firmware
5. Start simulation

## Testing Scenarios

### 1. Normal Operation Test

**Steps**:
1. Start simulation
2. Wait for OLED to show startup screen (3 seconds)
3. System should transition to NORMAL state
4. RGB LED should be **GREEN**
5. OLED should display voltage, current, and power readings

**Expected Behavior**:
- Voltage: ~180-250V (based on potentiometer position)
- Current: ~0-25A (based on potentiometer position)
- Power: Voltage × Current
- Display updates every 1 second

### 2. Over-Voltage Trip Test

**Steps**:
1. Wait for system to enter NORMAL state
2. Slowly turn the **voltage potentiometer** to maximum (right)
3. Watch for voltage reading to exceed 240V

**Expected Behavior**:
- Relay clicks (trips)
- RGB LED turns **RED**
- OLED displays "ALERT! OVER VOLTAGE"
- Serial monitor shows fault detection
- System enters TRIP_PROTECTION state

### 3. Over-Current Trip Test

**Steps**:
1. Reset simulation (or wait for Blynk reset)
2. Wait for NORMAL state
3. Turn the **current potentiometer** to maximum (right)
4. Watch for current reading to exceed 20A

**Expected Behavior**:
- Relay trips
- RGB LED turns **RED**
- OLED displays "ALERT! OVER CURRENT"
- System enters TRIP_PROTECTION state

### 4. Under-Voltage Trip Test

**Steps**:
1. Reset simulation
2. Wait for NORMAL state
3. Turn the **voltage potentiometer** to minimum (left)
4. Watch for voltage reading to drop below 180V

**Expected Behavior**:
- Relay trips
- RGB LED turns **RED**
- OLED displays "ALERT! UNDER VOLTAGE"
- System enters TRIP_PROTECTION state

### 5. Offline Mode Test

**Note**: WiFi simulation in Wokwi is limited. The system will likely enter OFFLINE_MODE automatically since there's no real WiFi network.

**Expected Behavior**:
- RGB LED turns **YELLOW**
- OLED shows "OFFLINE MODE"
- Safety monitoring continues
- System can still trip on faults

## Serial Monitor Commands

Watch the serial monitor for debug output:

```
[EnergySensor] V=220.50V, I=5.23A, P=1153.22W
[SafetyManager] All checks passed
[DisplayManager] Showing data screen
[NetworkManager] WiFi disconnected
[StateManager] State: OFFLINE_MODE
```

## Potentiometer Mapping

### Voltage Potentiometer (GPIO 34)
- **Minimum (Left)**: ~180V
- **Center**: ~215V
- **Maximum (Right)**: ~250V

### Current Potentiometer (GPIO 35)
- **Minimum (Left)**: ~0A
- **Center**: ~12.5A
- **Maximum (Right)**: ~25A

## Triggering Safety Trips

To test each safety feature:

| Fault Type | Potentiometer | Position | Threshold |
|------------|---------------|----------|-----------|
| Over-Voltage | Voltage | Maximum (Right) | >240V |
| Under-Voltage | Voltage | Minimum (Left) | <180V |
| Over-Current | Current | Maximum (Right) | >20A |

## Reset After Trip

Since the physical button was removed, you have two options to reset:

1. **Restart Simulation**: Stop and start the simulation
2. **Blynk Integration**: If you configure Blynk, use the V4 reset button

## Limitations in Simulation

⚠️ **What doesn't work in Wokwi**:
- WiFi connection (will always be in OFFLINE_MODE)
- Blynk communication
- Real-time cloud data logging

✅ **What works perfectly**:
- Sensor reading (via potentiometers)
- Safety threshold checking
- Relay trip mechanism
- RGB LED status indication
- OLED display updates
- State machine transitions
- Serial debug output

## Troubleshooting

### Display Not Showing
- Check I2C address in `config.h` (should be 0x3C)
- Verify OLED connections in `diagram.json`

### Relay Not Clicking
- Ensure potentiometer values exceed thresholds
- Check serial monitor for safety check messages

### RGB LED Not Changing
- Verify Common Cathode configuration in `config.h`
- Check resistor connections in diagram

### Simulation Crashes
- Reduce `ADC_SAMPLES` in `config.h` to 5 (from 10)
- Increase task intervals to reduce CPU load

## Advanced: Modifying the Simulation

### Change Voltage Range

Edit `EnergySensor.cpp`, line ~75:
```cpp
float simulatedVoltage = map(rawADC, 0, ADC_RESOLUTION, 180, 250);
//                                                        ^^^  ^^^
//                                                        min  max
```

### Change Current Range

Edit `EnergySensor.cpp`, line ~88:
```cpp
float simulatedCurrent = map(rawADC, 0, ADC_RESOLUTION, 0, 25);
//                                                       ^  ^^
//                                                      min max
```

### Add More Components

Edit `diagram.json` to add:
- Push buttons
- Additional LEDs
- LCD display
- Buzzers

## Video Recording

Wokwi allows you to record your simulation:
1. Start simulation
2. Click "Record" button
3. Perform test scenarios
4. Stop recording
5. Download video

Perfect for documentation and demonstrations!

## Next Steps

After validating in Wokwi:
1. ✅ Verify all state transitions work
2. ✅ Test safety thresholds
3. ✅ Confirm display updates
4. 🔧 Build on real hardware
5. 🔧 Calibrate with actual sensors
6. 🔧 Configure WiFi and Blynk

---

**Happy Simulating!** 🚀

For issues or questions, check the serial monitor output first - it provides detailed debug information about what the system is doing.
