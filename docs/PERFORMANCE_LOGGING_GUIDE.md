# Performance Evaluation System - Usage Guide

## Overview

The performance evaluation system has been successfully implemented! It measures three critical metrics:

1. **Data Transmission Latency** - Sensor read → Blynk transmission time
2. **Measurement Accuracy** - Voltage/current stability over time
3. **Trip Response Time** - Fault detection → Relay trip time

## Quick Start

### Step 1: Enable Performance Logging

Edit `include/config.h` and uncomment the performance logging flag:

```cpp
// Enable performance logging (uncomment to enable)
#define ENABLE_PERFORMANCE_LOGGING  // <-- Uncomment this line
```

### Step 2: Build and Upload

```bash
# Build the project
pio run

# Upload to ESP32
pio run -t upload
```

### Step 3: Start PC Logger

Open a terminal and run:

```bash
# Windows
python tools/performance_logger.py COM3 115200

# Linux/Mac
python tools/performance_logger.py /dev/ttyUSB0 115200
```

**Note:** Replace `COM3` with your actual COM port. Check Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac).

### Step 4: Collect Data

Let the system run for your desired duration:
- **Quick test**: 5-10 minutes
- **Full evaluation**: 1-24 hours
- **Stress test**: Vary loads, trigger faults

### Step 5: Stop and Analyze

Press `Ctrl+C` in the Python script to stop logging. CSV files will be saved in the `performance_data/` directory.

## CSV File Format

### latency_YYYYMMDD_HHMMSS.csv

```csv
Timestamp(ms),SensorRead(us),BlynkTransmit(ms),TotalLatency(ms)
1523,245123,15,260
3045,243987,14,258
4567,246234,16,262
```

**Columns:**
- `Timestamp(ms)`: System uptime in milliseconds
- `SensorRead(us)`: Sensor reading time in microseconds
- `BlynkTransmit(ms)`: Blynk transmission time in milliseconds
- `TotalLatency(ms)`: Total latency (sensor + Blynk)

### accuracy_YYYYMMDD_HHMMSS.csv

```csv
Timestamp(ms),Voltage(V),Current(A),Power(W)
10234,223.45,5.23,1168.62
20456,223.51,5.21,1164.49
30678,223.48,5.22,1166.57
```

**Columns:**
- `Timestamp(ms)`: System uptime in milliseconds
- `Voltage(V)`: Voltage reading in Volts
- `Current(A)`: Current reading in Amperes
- `Power(W)`: Power reading in Watts

**Logged every 10 seconds** (configurable in `config.h`)

### trip_response_YYYYMMDD_HHMMSS.csv

```csv
Timestamp(ms),FaultDetect(us),RelayTrip(us),TotalResponse(ms)
45123,125,85,0.21
```

**Columns:**
- `Timestamp(ms)`: System uptime in milliseconds
- `FaultDetect(us)`: Fault detection time in microseconds
- `RelayTrip(us)`: Relay trip time in microseconds
- `TotalResponse(ms)`: Total response time (detect + trip)

**Logged only when faults occur**

## Analyzing Results

### Using Excel/Google Sheets

1. Open CSV file in Excel/Google Sheets
2. Calculate statistics:
   ```excel
   Average Latency: =AVERAGE(D:D)
   Max Latency: =MAX(D:D)
   Min Latency: =MIN(D:D)
   Std Dev: =STDEV(D:D)
   ```
3. Create graphs:
   - Line chart: Latency over time
   - Line chart: Voltage/Current stability
   - Histogram: Trip response distribution

### Using Python (pandas)

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load data
latency = pd.read_csv('performance_data/latency_20260106_183045.csv')
accuracy = pd.read_csv('performance_data/accuracy_20260106_183045.csv')

# Calculate statistics
print("Latency Statistics:")
print(latency['TotalLatency(ms)'].describe())

print("\nVoltage Statistics:")
print(accuracy['Voltage(V)'].describe())

# Plot latency over time
plt.figure(figsize=(12, 6))
plt.plot(latency['Timestamp(ms)'], latency['TotalLatency(ms)'])
plt.xlabel('Time (ms)')
plt.ylabel('Latency (ms)')
plt.title('Data Transmission Latency Over Time')
plt.grid(True)
plt.show()

# Plot voltage stability
plt.figure(figsize=(12, 6))
plt.plot(accuracy['Timestamp(ms)'], accuracy['Voltage(V)'])
plt.xlabel('Time (ms)')
plt.ylabel('Voltage (V)')
plt.title('Voltage Stability Over Time')
plt.grid(True)
plt.show()
```

## Performance Targets

### ✅ Success Criteria

| Metric | Target | Acceptable Range |
|--------|--------|------------------|
| **Total Latency** | < 100ms | 50-150ms |
| **Sensor Read Time** | < 250ms | 200-300ms |
| **Blynk Transmit** | < 20ms | 10-30ms |
| **Voltage Variation** | < 5% | 2-8% |
| **Current Variation** | < 5% | 2-8% |
| **Trip Response** | < 10ms | 5-15ms |

## Troubleshooting

### Python Script Issues

**Problem:** `ModuleNotFoundError: No module named 'serial'`

**Solution:**
```bash
pip install pyserial
```

**Problem:** `PermissionError: [Errno 13] Permission denied: 'COM3'`

**Solution:**
- Close Arduino IDE or any other program using the serial port
- On Linux: `sudo chmod 666 /dev/ttyUSB0`

**Problem:** No data appearing in CSV files

**Solution:**
- Check that `ENABLE_PERFORMANCE_LOGGING` is uncommented in `config.h`
- Verify ESP32 is running (check for debug output)
- Check baud rate matches (115200)

### ESP32 Issues

**Problem:** Build errors after enabling performance logging

**Solution:**
- Ensure `PerformanceLogger.h` and `PerformanceLogger.cpp` are in correct directories
- Clean build: `pio run -t clean` then `pio run`

**Problem:** High memory usage

**Solution:**
- Performance logging uses minimal memory (~200 bytes)
- If issues persist, disable logging for production use

## Configuration Options

### Accuracy Logging Interval

Edit `include/config.h`:

```cpp
#ifdef ENABLE_PERFORMANCE_LOGGING
    #define PERF_ACCURACY_INTERVAL_MS  10000  // Change to desired interval (ms)
#endif
```

**Recommendations:**
- Fast logging (1 second): `1000` - for short tests
- Normal logging (10 seconds): `10000` - for general use
- Slow logging (1 minute): `60000` - for long-term monitoring

## Test Scenarios

### Baseline Test (10 minutes)

**Purpose:** Establish normal operating parameters

**Procedure:**
1. Enable performance logging
2. Upload to ESP32
3. Run Python logger for 10 minutes
4. No load changes during test

**Expected Results:**
- Stable voltage/current readings
- Consistent latency values
- No trip events

### Load Variation Test (30 minutes)

**Purpose:** Test accuracy under varying loads

**Procedure:**
1. Start with no load
2. Add 10W load at 5 minutes
3. Add 50W load at 15 minutes
4. Remove all loads at 25 minutes

**Expected Results:**
- Current readings should track load changes
- Voltage should remain stable
- Latency should remain consistent

### Fault Response Test

**Purpose:** Measure trip response time

**Procedure:**
1. Enable performance logging
2. Trigger over-voltage condition (if safe)
3. Observe trip response in CSV

**Expected Results:**
- Trip response < 10ms
- Fault detection < 200µs
- Relay trip < 100µs

## Disabling Performance Logging

To disable for production use:

1. Comment out the flag in `config.h`:
   ```cpp
   // #define ENABLE_PERFORMANCE_LOGGING
   ```

2. Rebuild and upload:
   ```bash
   pio run -t upload
   ```

**Note:** All performance logging code is removed at compile time (zero overhead).

## Summary

The performance evaluation system is now fully implemented and ready to use. Simply:

1. ✅ Uncomment `ENABLE_PERFORMANCE_LOGGING` in `config.h`
2. ✅ Build and upload to ESP32
3. ✅ Run `python tools/performance_logger.py COM3 115200`
4. ✅ Analyze CSV files in `performance_data/` directory

For questions or issues, refer to the implementation plan or contact support.
