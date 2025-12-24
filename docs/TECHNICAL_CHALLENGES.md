# Technical Challenges Report: Energy Monitoring & Safety System

## Project Overview
Development of an ESP32-based energy monitoring system with safety trip protection using ZMPT101B voltage sensor and SCT-013 current sensor.

---

## Challenge 1: Persistent Under-Voltage Trip Alerts

### Problem Description
The system continuously triggered "UNDER VOLTAGE" alerts even when mains voltage was normal (228V according to multimeter).

### Root Cause
- Initial calibration factor (550.0) was too low
- Device readings showed ~216V while actual voltage was 228V
- Readings fell below the `VOLTAGE_MIN` threshold (216V), causing false trips

### Solution Implemented
- Added debug logging to display actual sensor readings
- Calculated correct calibration factor: `Real Voltage / Raw RMS = 228 / 0.41 ≈ 556`
- Updated `VOLTAGE_CALIBRATION_FACTOR` from 550.0 → 583.0 → 607.0 → 621.0 → 556.0

### Lessons Learned
- Calibration must be done with actual multimeter readings, not assumptions
- Raw sensor values (RMS) are essential for accurate calibration
- Iterative calibration is necessary as hardware changes affect readings

---

## Challenge 2: Ghost Voltage Readings When Mains OFF

### Problem Description
When mains power was disconnected, the sensor read ~64V instead of 0V.

### Root Cause
- Electrical noise and EMI picked up by the sensor circuit
- Hardcoded noise threshold (50V) was too low to filter out this interference

### Solution Implemented
- Increased `VOLTAGE_NOISE_THRESHOLD` from 50V to 75V
- Added noise gate logic: readings below threshold are set to 0V
- Implemented dynamic DC bias calculation instead of assuming fixed 2.5V center point

### Lessons Learned
- AC sensors are susceptible to environmental noise
- Noise thresholds must be calibrated based on actual environmental conditions
- Never assume fixed DC bias in real-world analog circuits

---

## Challenge 3: Static Current Reading (15A Constant)

### Problem Description
Current sensor showed constant 15A regardless of actual load or whether circuit was energized.

### Root Cause
- Simulation mode was using `map()` function to generate fake data
- Code was: `map(rawADC, 0, 4095, 0, 25)` which linearly mapped any noise to 0-25A range
- No actual RMS calculation was being performed

### Solution Implemented
- Rewrote `readCurrentSimulation()` to use proper RMS calculation
- Implemented two-pass algorithm:
  - Pass 1: Calculate DC bias (mean of samples)
  - Pass 2: Calculate RMS relative to DC bias
- Added `CURRENT_NOISE_THRESHOLD` (0.60A) to filter residual noise

### Lessons Learned
- Simulation/test code must still implement correct algorithms
- RMS calculation requires removing DC offset before computing root-mean-square
- Current sensors (SCT-013) require same mathematical rigor as voltage sensors

---

## Challenge 4: Voltage Reading Fluctuations

### Problem Description
Voltage readings fluctuated wildly (211V → 223V → 215V), causing intermittent trips.

### Root Cause Analysis
1. **Hardcoded DC Bias**: Code assumed signal centered at 2.5V, but actual center drifted
2. **Two-Pass Timing Error**: Measuring DC bias in first 100ms, then RMS in second 100ms allowed signal changes between passes
3. **ESP32 ADC Noise**: WiFi activity and other processes introduced timing jitter

### Solution Implemented
**Phase 1: Dynamic DC Bias**
- Replaced hardcoded 2.5V with calculated mean from actual samples
- Reduced errors from DC drift

**Phase 2: Single-Pass Algorithm**
- Implemented true RMS formula: `RMS = sqrt((SumSq - (Sum²/N)) / N)`
- Calculates mean and RMS simultaneously in one sampling loop
- Doubled sample count (500 → 1000) for better averaging
- Used double precision for intermediate calculations to prevent overflow

**Phase 3: Exponential Moving Average (EMA)**
- Added smoothing filter: `smoothed = 0.2 × new + 0.8 × old`
- Reduced impact of transient spikes

### Lessons Learned
- Multi-pass algorithms are vulnerable to timing errors in real-time systems
- Single-pass algorithms are more robust for AC signal processing
- Smoothing filters must be carefully tuned to balance responsiveness vs. stability

---

## Challenge 5: False Trips During Intentional Power-Off

### Problem Description
System entered TRIP state when user turned off mains power, requiring manual reset even after power restoration.

### Root Cause
Safety logic couldn't distinguish between:
- **Intentional power-off**: 0V (normal user action)
- **Brown-out fault**: 200V during operation (actual fault)

Both triggered `voltage < VOLTAGE_MIN` condition.

### Solution Implemented
**Power Present Detection**
```c
#define VOLTAGE_POWER_PRESENT_THRESHOLD 100.0
```

Modified `SafetyManager::checkSafety()`:
1. If `voltage < 100V`: Power is OFF → Skip voltage checks → Return safe
2. If `voltage > 100V`: Power is ON → Check normal limits (216-244V)

### Lessons Learned
- Safety systems must consider operational context, not just absolute thresholds
- State machines need to distinguish between normal state transitions and fault conditions
- User experience matters: false alarms reduce trust in safety systems

---

## Challenge 6: EMA Smoothing Causing Delayed Power-Off Detection

### Problem Description
After implementing EMA smoothing, turning off power caused trips during the voltage decay period (228V → 180V → 140V → 100V → 0V).

### Root Cause
- EMA filter has exponential decay with time constant τ = 1/α
- With α = 0.2, decay takes ~5 time constants ≈ 5 seconds
- During decay, voltage passed through the trip zone (100-216V)

### Solution Implemented
Modified smoothing logic to reset immediately when raw voltage drops below noise threshold:
```cpp
if (rawVoltage < VOLTAGE_NOISE_THRESHOLD) {
    voltage_ = 0.0;  // Immediate reset
} else {
    voltage_ = 0.2 × rawVoltage + 0.8 × voltage_;  // Normal smoothing
}
```

### Lessons Learned
- Smoothing filters must handle edge cases (power on/off transitions)
- Filters designed for steady-state operation may fail during transients
- Hybrid approaches (immediate reset + gradual smoothing) provide best results

---

## Challenge 7: ZMPT101B Gain Calibration Issues

### Problem Description
After adjusting physical potentiometer, raw RMS dropped to 0.04-0.06V, causing all readings to be zeroed out by noise gate.

### Root Cause
- User reduced gain too much trying to prevent signal clipping
- Raw RMS × Calibration Factor < Noise Threshold → All readings zeroed

### Ongoing Investigation
- Potentiometer adjustment had no effect on readings
- Possible causes:
  - Wrong potentiometer being adjusted (gain vs. offset)
  - Wiring issue between ZMPT101B and ESP32
  - Insufficient AC input to ZMPT101B
  - Damaged sensor

### Lessons Learned
- Hardware calibration requires clear documentation of which controls affect what
- Software should provide diagnostic feedback during hardware adjustment
- Sensor validation should be performed before software calibration

---

## Summary of Solutions Implemented

| Challenge | Solution | Files Modified |
|-----------|----------|----------------|
| Under-voltage trips | Calibration factor adjustment | `config.h` |
| Ghost voltage readings | Noise threshold + dynamic DC bias | `config.h`, `EnergySensor.cpp` |
| Static current reading | Proper RMS calculation | `EnergySensor.cpp` |
| Voltage fluctuations | Single-pass RMS + EMA smoothing | `EnergySensor.cpp` |
| False power-off trips | Power present detection | `config.h`, `SafetyManager.cpp` |
| EMA decay trips | Conditional smoothing reset | `EnergySensor.cpp` |
| Relay state visibility | Added relay status to logs | `StateManager.cpp` |
| Blynk updates during trip | Added publishData to trip state | `StateManager.cpp` |

---

## Key Technical Insights

### AC Signal Processing
1. **RMS Calculation**: Must remove DC bias before computing root-mean-square
2. **Sampling**: Minimum 500 samples over multiple AC cycles (50Hz = 20ms period)
3. **Precision**: Use double precision for intermediate calculations to prevent overflow

### Sensor Calibration
1. **Formula**: `Calibration Factor = Real Value / Raw RMS`
2. **Process**: Measure with calibrated reference → Read raw sensor → Calculate factor
3. **Iteration**: Hardware changes require recalibration

### Safety System Design
1. **Context Awareness**: Distinguish between faults and normal state transitions
2. **Hysteresis**: Prevent chattering with separate trip and reset thresholds
3. **User Experience**: Minimize false alarms while maintaining protection

### Embedded Systems
1. **Timing**: Single-pass algorithms more robust than multi-pass in real-time systems
2. **Filtering**: Balance between noise rejection and responsiveness
3. **Diagnostics**: Raw sensor logging essential for debugging and calibration

---

## Recommendations for Future Work

1. **Hardware Validation Suite**: Automated tests to verify sensor connectivity and basic operation
2. **Calibration Wizard**: Step-by-step UI to guide users through sensor calibration
3. **Adaptive Thresholds**: Machine learning to adjust noise thresholds based on environment
4. **Redundant Sensors**: Multiple voltage sensors for cross-validation
5. **Waveform Capture**: Store raw ADC samples for post-analysis of trip events
6. **Self-Test Mode**: Periodic verification of sensor operation without tripping

---

## Conclusion

This project demonstrated the complexity of real-world embedded systems development, where theoretical algorithms must be adapted to handle hardware imperfections, environmental noise, and user interaction patterns. The iterative problem-solving process—from initial debug logging through algorithm optimization to user experience refinement—highlights the importance of systematic debugging, comprehensive testing, and maintaining flexibility to revise assumptions when confronted with empirical data.
