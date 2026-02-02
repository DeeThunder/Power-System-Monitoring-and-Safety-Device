# Performance Logging & Data Collection Guide

## Overview

This guide explains how to log and evaluate system performance metrics:
1. **Data Transmission Latency** - Time from sensor reading to cloud delivery
2. **Measurement Accuracy** - Comparison with reference instruments
3. **Trip Response Time** - Time from fault detection to relay disconnection

---

## 1. Performance Logging Implementation

### Step 1: Add Performance Logger Class

Create `include/PerformanceLogger.h`:

```cpp
#ifndef PERFORMANCE_LOGGER_H
#define PERFORMANCE_LOGGER_H

#include <Arduino.h>

class PerformanceLogger {
public:
    void begin();
    
    // Latency tracking
    void markSensorReadStart();
    void markSensorReadEnd();
    void markBlynkPublishStart();
    void markBlynkPublishEnd();
    
    // Accuracy tracking
    void logAccuracyTest(float measured, float reference, const String& parameter);
    
    // Trip response tracking
    void markFaultDetected();
    void markRelayTripped();
    
    // Data export
    void printCSVHeader();
    void printCSVRow();
    void printSummary();
    
private:
    // Timing variables
    unsigned long sensorReadStartTime_;
    unsigned long sensorReadEndTime_;
    unsigned long blynkPublishStartTime_;
    unsigned long blynkPublishEndTime_;
    unsigned long faultDetectedTime_;
    unsigned long relayTrippedTime_;
    
    // Statistics
    float totalSensorReadTime_;
    float totalBlynkLatency_;
    float totalTripResponseTime_;
    int sensorReadCount_;
    int blynkPublishCount_;
    int tripCount_;
    
    // Accuracy tracking
    struct AccuracyRecord {
        String parameter;
        float measured;
        float reference;
        float error;
        float errorPercent;
        unsigned long timestamp;
    };
    
    static const int MAX_ACCURACY_RECORDS = 100;
    AccuracyRecord accuracyRecords_[MAX_ACCURACY_RECORDS];
    int accuracyRecordCount_;
};

#endif
```

Create `src/PerformanceLogger.cpp`:

```cpp
#include "PerformanceLogger.h"

void PerformanceLogger::begin() {
    sensorReadStartTime_ = 0;
    sensorReadEndTime_ = 0;
    blynkPublishStartTime_ = 0;
    blynkPublishEndTime_ = 0;
    faultDetectedTime_ = 0;
    relayTrippedTime_ = 0;
    
    totalSensorReadTime_ = 0;
    totalBlynkLatency_ = 0;
    totalTripResponseTime_ = 0;
    sensorReadCount_ = 0;
    blynkPublishCount_ = 0;
    tripCount_ = 0;
    accuracyRecordCount_ = 0;
    
    Serial.println("[PerformanceLogger] Initialized");
}

void PerformanceLogger::markSensorReadStart() {
    sensorReadStartTime_ = micros();
}

void PerformanceLogger::markSensorReadEnd() {
    sensorReadEndTime_ = micros();
    
    if (sensorReadStartTime_ > 0) {
        float duration = (sensorReadEndTime_ - sensorReadStartTime_) / 1000.0; // ms
        totalSensorReadTime_ += duration;
        sensorReadCount_++;
        
        Serial.printf("[PERF] Sensor Read: %.2f ms\n", duration);
    }
}

void PerformanceLogger::markBlynkPublishStart() {
    blynkPublishStartTime_ = millis();
}

void PerformanceLogger::markBlynkPublishEnd() {
    blynkPublishEndTime_ = millis();
    
    if (blynkPublishStartTime_ > 0) {
        float latency = blynkPublishEndTime_ - blynkPublishStartTime_;
        totalBlynkLatency_ += latency;
        blynkPublishCount_++;
        
        Serial.printf("[PERF] Blynk Latency: %.0f ms\n", latency);
    }
}

void PerformanceLogger::logAccuracyTest(float measured, float reference, const String& parameter) {
    if (accuracyRecordCount_ >= MAX_ACCURACY_RECORDS) {
        Serial.println("[PERF] Accuracy log full!");
        return;
    }
    
    AccuracyRecord& record = accuracyRecords_[accuracyRecordCount_];
    record.parameter = parameter;
    record.measured = measured;
    record.reference = reference;
    record.error = measured - reference;
    record.errorPercent = (record.error / reference) * 100.0;
    record.timestamp = millis();
    
    accuracyRecordCount_++;
    
    Serial.printf("[PERF] Accuracy - %s: Measured=%.2f, Reference=%.2f, Error=%.2f (%.2f%%)\n",
                  parameter.c_str(), measured, reference, record.error, record.errorPercent);
}

void PerformanceLogger::markFaultDetected() {
    faultDetectedTime_ = micros();
    Serial.printf("[PERF] Fault detected at: %lu us\n", faultDetectedTime_);
}

void PerformanceLogger::markRelayTripped() {
    relayTrippedTime_ = micros();
    
    if (faultDetectedTime_ > 0) {
        float responseTime = (relayTrippedTime_ - faultDetectedTime_) / 1000.0; // ms
        totalTripResponseTime_ += responseTime;
        tripCount_++;
        
        Serial.printf("[PERF] Trip Response Time: %.3f ms\n", responseTime);
    }
}

void PerformanceLogger::printCSVHeader() {
    Serial.println("\n=== CSV DATA START ===");
    Serial.println("Timestamp,Parameter,Measured,Reference,Error,ErrorPercent");
}

void PerformanceLogger::printCSVRow() {
    for (int i = 0; i < accuracyRecordCount_; i++) {
        AccuracyRecord& r = accuracyRecords_[i];
        Serial.printf("%lu,%s,%.3f,%.3f,%.3f,%.3f\n",
                      r.timestamp, r.parameter.c_str(), r.measured, 
                      r.reference, r.error, r.errorPercent);
    }
    Serial.println("=== CSV DATA END ===\n");
}

void PerformanceLogger::printSummary() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║         PERFORMANCE EVALUATION SUMMARY                 ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    
    // Sensor Read Performance
    Serial.println("\n📊 SENSOR READ PERFORMANCE:");
    if (sensorReadCount_ > 0) {
        float avgSensorTime = totalSensorReadTime_ / sensorReadCount_;
        Serial.printf("  • Total Readings: %d\n", sensorReadCount_);
        Serial.printf("  • Average Time: %.2f ms\n", avgSensorTime);
        Serial.printf("  • Total Time: %.2f ms\n", totalSensorReadTime_);
    } else {
        Serial.println("  • No data collected");
    }
    
    // Data Transmission Latency
    Serial.println("\n📡 DATA TRANSMISSION LATENCY:");
    if (blynkPublishCount_ > 0) {
        float avgLatency = totalBlynkLatency_ / blynkPublishCount_;
        Serial.printf("  • Total Transmissions: %d\n", blynkPublishCount_);
        Serial.printf("  • Average Latency: %.0f ms\n", avgLatency);
        Serial.printf("  • Min Expected: ~100 ms (local network)\n");
        Serial.printf("  • Max Acceptable: <500 ms\n");
        
        if (avgLatency < 200) {
            Serial.println("  ✅ EXCELLENT - Very low latency");
        } else if (avgLatency < 500) {
            Serial.println("  ✅ GOOD - Acceptable latency");
        } else {
            Serial.println("  ⚠️  WARNING - High latency detected");
        }
    } else {
        Serial.println("  • No data collected");
    }
    
    // Measurement Accuracy
    Serial.println("\n🎯 MEASUREMENT ACCURACY:");
    if (accuracyRecordCount_ > 0) {
        float sumErrorPercent = 0;
        for (int i = 0; i < accuracyRecordCount_; i++) {
            sumErrorPercent += abs(accuracyRecords_[i].errorPercent);
        }
        float avgErrorPercent = sumErrorPercent / accuracyRecordCount_;
        
        Serial.printf("  • Total Measurements: %d\n", accuracyRecordCount_);
        Serial.printf("  • Average Error: %.2f%%\n", avgErrorPercent);
        
        if (avgErrorPercent < 2.0) {
            Serial.println("  ✅ EXCELLENT - High accuracy");
        } else if (avgErrorPercent < 5.0) {
            Serial.println("  ✅ GOOD - Acceptable accuracy");
        } else {
            Serial.println("  ⚠️  WARNING - Calibration needed");
        }
    } else {
        Serial.println("  • No data collected");
    }
    
    // Trip Response Time
    Serial.println("\n⚡ TRIP RESPONSE TIME:");
    if (tripCount_ > 0) {
        float avgResponseTime = totalTripResponseTime_ / tripCount_;
        Serial.printf("  • Total Trips: %d\n", tripCount_);
        Serial.printf("  • Average Response: %.3f ms\n", avgResponseTime);
        Serial.printf("  • Target: <10 ms\n");
        
        if (avgResponseTime < 5.0) {
            Serial.println("  ✅ EXCELLENT - Very fast response");
        } else if (avgResponseTime < 10.0) {
            Serial.println("  ✅ GOOD - Acceptable response");
        } else {
            Serial.println("  ⚠️  WARNING - Slow response detected");
        }
    } else {
        Serial.println("  • No trip events recorded");
    }
    
    Serial.println("\n════════════════════════════════════════════════════════\n");
}
```

---

## 2. Integration into Existing Code

### Update `main.cpp`:

```cpp
#include "PerformanceLogger.h"

// Add to global objects
PerformanceLogger perfLogger;

void setup() {
    // ... existing setup code ...
    
    perfLogger.begin();
    
    // Print CSV header for data collection
    perfLogger.printCSVHeader();
}

void loop() {
    stateManager.update();
    
    // Print summary every 5 minutes
    static unsigned long lastSummary = 0;
    if (millis() - lastSummary > 300000) {  // 5 minutes
        lastSummary = millis();
        perfLogger.printSummary();
    }
    
    yield();
}
```

### Update `EnergySensor.cpp`:

```cpp
// Add to update() method
void EnergySensor::update() {
    extern PerformanceLogger perfLogger;  // Declare external
    
    perfLogger.markSensorReadStart();
    
    if (simulationMode_) {
        // ... existing code ...
    }
    
    perfLogger.markSensorReadEnd();
}
```

### Update `NetworkManager.cpp`:

```cpp
void NetworkManager::publishData(float voltage, float current, float power) {
    extern PerformanceLogger perfLogger;
    
    if (!blynkConnected_) return;
    
    unsigned long now = millis();
    if (now - lastBlynkUpdate_ < INTERVAL_BLYNK) return;
    
    lastBlynkUpdate_ = now;
    
    perfLogger.markBlynkPublishStart();
    
    Blynk.virtualWrite(VPIN_VOLTAGE, voltage);
    Blynk.virtualWrite(VPIN_CURRENT, current);
    Blynk.virtualWrite(VPIN_POWER, power);
    
    perfLogger.markBlynkPublishEnd();
    
    // ... rest of code ...
}
```

### Update `SafetyManager.cpp`:

```cpp
bool SafetyManager::checkSafety(float voltage, float current) {
    extern PerformanceLogger perfLogger;
    
    // ... existing power detection code ...
    
    // Check over-voltage
    if (checkOverVoltage(voltage)) {
        perfLogger.markFaultDetected();
        lastFaultReason_ = "OVER VOLTAGE";
        return false;
    }
    
    // ... similar for other checks ...
}

void SafetyManager::tripRelay() {
    extern PerformanceLogger perfLogger;
    
    digitalWrite(PIN_RELAY, !RELAY_ACTIVE_STATE);
    relayTripped_ = true;
    setRGBStatus(RGB_RED);
    
    perfLogger.markRelayTripped();
    
    // ... rest of code ...
}
```

---

## 3. Data Collection Methods

### Method 1: Serial Monitor Logging (Recommended for Testing)

**Step 1: Configure PlatformIO**

Add to `platformio.ini`:
```ini
[env:esp32dev]
monitor_speed = 115200
monitor_filters = 
    default
    time
    log2file
```

**Step 2: Capture Serial Output**

```bash
# Using PlatformIO CLI
pio device monitor > performance_log.txt

# Or use screen (Linux/Mac)
screen -L /dev/ttyUSB0 115200

# Or use PuTTY (Windows) with logging enabled
```

**Step 3: Parse CSV Data**

The logger outputs CSV format between markers:
```
=== CSV DATA START ===
Timestamp,Parameter,Measured,Reference,Error,ErrorPercent
12345,Voltage,228.50,230.00,-1.50,-0.65
12456,Current,12.30,12.50,-0.20,-1.60
=== CSV DATA END ===
```

Extract with Python:
```python
import re

with open('performance_log.txt', 'r') as f:
    content = f.read()
    
# Extract CSV data
csv_pattern = r'=== CSV DATA START ===\n(.*?)\n=== CSV DATA END ==='
matches = re.findall(csv_pattern, content, re.DOTALL)

for match in matches:
    with open('accuracy_data.csv', 'w') as csv_file:
        csv_file.write(match)
```

### Method 2: SD Card Logging (For Long-Term Data)

**Hardware**: Add SD card module to ESP32

**Implementation**:

```cpp
#include <SD.h>
#include <SPI.h>

#define SD_CS_PIN 5

class SDLogger {
public:
    bool begin() {
        if (!SD.begin(SD_CS_PIN)) {
            Serial.println("SD Card initialization failed!");
            return false;
        }
        
        // Create log file with timestamp
        logFile_ = SD.open("/performance.csv", FILE_WRITE);
        if (logFile_) {
            logFile_.println("Timestamp,Voltage,Current,Power,Latency");
            logFile_.close();
            return true;
        }
        return false;
    }
    
    void logData(float voltage, float current, float power, float latency) {
        logFile_ = SD.open("/performance.csv", FILE_APPEND);
        if (logFile_) {
            logFile_.printf("%lu,%.2f,%.2f,%.2f,%.0f\n", 
                           millis(), voltage, current, power, latency);
            logFile_.close();
        }
    }
    
private:
    File logFile_;
};
```

### Method 3: Blynk Data Export

**Option A: Blynk Datastreams History**

1. Open Blynk Console
2. Go to Device → Datastreams
3. Click on datastream (e.g., V0 - Voltage)
4. Click "Export" button
5. Download CSV file

**Option B: Blynk API**

```python
import requests
import pandas as pd

BLYNK_TOKEN = "YourAuthToken"
DEVICE_ID = "YourDeviceID"
PIN = "V0"  # Voltage pin

# Get data for last 24 hours
url = f"https://blynk.cloud/external/api/data/get?token={BLYNK_TOKEN}&pin={PIN}&period=DAY"

response = requests.get(url)
data = response.json()

# Convert to DataFrame
df = pd.DataFrame(data)
df.to_csv('blynk_voltage_data.csv', index=False)
```

---

## 4. Performance Testing Procedures

### Test 1: Data Transmission Latency

**Objective**: Measure time from sensor reading to cloud delivery

**Procedure**:
1. Enable performance logging
2. Run system for 1 hour
3. Collect logs
4. Calculate average latency

**Expected Results**:
- Local network: 100-200ms
- Internet: 200-500ms
- Acceptable: <500ms

**Code to Add**:
```cpp
// In main loop
static unsigned long testStartTime = 0;
static int testDuration = 3600000;  // 1 hour

if (millis() - testStartTime < testDuration) {
    // Test running
} else if (testStartTime > 0) {
    // Test complete
    perfLogger.printSummary();
    perfLogger.printCSVRow();
    testStartTime = 0;  // Reset
}
```

### Test 2: Measurement Accuracy

**Objective**: Compare device readings with calibrated instruments

**Equipment Needed**:
- Calibrated multimeter (True RMS)
- Calibrated clamp meter
- Stable AC source

**Procedure**:
1. Connect multimeter in parallel
2. Connect clamp meter on same wire
3. Record 50 samples over 10 minutes
4. Log each comparison

**Code**:
```cpp
// Add to main loop for manual testing
void runAccuracyTest() {
    extern PerformanceLogger perfLogger;
    
    // Manually enter reference values from multimeter
    float referenceVoltage = 230.0;  // From multimeter
    float referenceCurrent = 12.5;   // From clamp meter
    
    float measuredVoltage = energySensor.getVoltage();
    float measuredCurrent = energySensor.getCurrent();
    
    perfLogger.logAccuracyTest(measuredVoltage, referenceVoltage, "Voltage");
    perfLogger.logAccuracyTest(measuredCurrent, referenceCurrent, "Current");
}

// Call every 10 seconds during test
static unsigned long lastAccuracyTest = 0;
if (millis() - lastAccuracyTest > 10000) {
    lastAccuracyTest = millis();
    runAccuracyTest();
}
```

### Test 3: Trip Response Time

**Objective**: Measure fault detection to relay trip time

**Procedure**:
1. Set up controlled fault condition
2. Trigger fault (e.g., increase voltage above threshold)
3. Measure time to relay trip
4. Repeat 10 times

**Expected Results**:
- Target: <10ms
- Excellent: <5ms
- Acceptable: <20ms

**Automated Test**:
```cpp
void runTripResponseTest() {
    // Simulate over-voltage condition
    // This would be done with variable transformer in real test
    
    Serial.println("\n=== TRIP RESPONSE TEST ===");
    Serial.println("Manually trigger fault condition now...");
    
    // System will automatically log when fault detected and relay trips
    // Check serial monitor for [PERF] Trip Response Time
}
```

---

## 5. Data Analysis

### Python Analysis Script

```python
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load data
df = pd.read_csv('accuracy_data.csv')

# Calculate statistics
voltage_data = df[df['Parameter'] == 'Voltage']
current_data = df[df['Parameter'] == 'Current']

print("=== VOLTAGE ACCURACY ===")
print(f"Mean Error: {voltage_data['Error'].mean():.2f} V")
print(f"Std Dev: {voltage_data['Error'].std():.2f} V")
print(f"Mean Error %: {voltage_data['ErrorPercent'].mean():.2f}%")

print("\n=== CURRENT ACCURACY ===")
print(f"Mean Error: {current_data['Error'].mean():.2f} A")
print(f"Std Dev: {current_data['Error'].std():.2f} A")
print(f"Mean Error %: {current_data['ErrorPercent'].mean():.2f}%")

# Plot error distribution
plt.figure(figsize=(12, 5))

plt.subplot(1, 2, 1)
plt.hist(voltage_data['ErrorPercent'], bins=20, edgecolor='black')
plt.xlabel('Error (%)')
plt.ylabel('Frequency')
plt.title('Voltage Measurement Error Distribution')
plt.axvline(0, color='red', linestyle='--', label='Zero Error')
plt.legend()

plt.subplot(1, 2, 2)
plt.hist(current_data['ErrorPercent'], bins=20, edgecolor='black')
plt.xlabel('Error (%)')
plt.ylabel('Frequency')
plt.title('Current Measurement Error Distribution')
plt.axvline(0, color='red', linestyle='--', label='Zero Error')
plt.legend()

plt.tight_layout()
plt.savefig('accuracy_analysis.png')
plt.show()
```

---

## 6. Report Template

### Performance Evaluation Report

```markdown
# System Performance Evaluation

## 1. Data Transmission Latency

| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| Average Latency | 245 ms | <500 ms | ✅ PASS |
| Min Latency | 120 ms | - | - |
| Max Latency | 480 ms | - | - |
| Total Samples | 1,234 | - | - |

**Analysis**: The system achieves acceptable latency with an average of 245ms, well within the 500ms target.

## 2. Measurement Accuracy

### Voltage Accuracy
| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| Mean Error | -1.2 V | ±5 V | ✅ PASS |
| Mean Error % | -0.52% | ±2% | ✅ PASS |
| Std Deviation | 0.8 V | - | - |

### Current Accuracy
| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| Mean Error | +0.3 A | ±1 A | ✅ PASS |
| Mean Error % | +2.4% | ±5% | ✅ PASS |
| Std Deviation | 0.5 A | - | - |

**Analysis**: Both voltage and current measurements meet accuracy requirements.

## 3. Trip Response Time

| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| Average Response | 4.2 ms | <10 ms | ✅ PASS |
| Min Response | 3.8 ms | - | - |
| Max Response | 5.1 ms | - | - |
| Total Trips | 10 | - | - |

**Analysis**: Trip response time is excellent, averaging 4.2ms.

## Conclusion

The system meets all performance targets:
- ✅ Data transmission latency acceptable
- ✅ Measurement accuracy within specifications
- ✅ Trip response time excellent
```

---

## Quick Start Checklist

- [ ] Add `PerformanceLogger` class to project
- [ ] Integrate logging calls in existing code
- [ ] Configure serial monitor logging
- [ ] Run 1-hour latency test
- [ ] Perform accuracy test with multimeter (50 samples)
- [ ] Conduct trip response test (10 trials)
- [ ] Export data from serial monitor
- [ ] Analyze data with Python script
- [ ] Generate performance report
- [ ] Include graphs and statistics in final report

---

**Note**: For academic evaluation, collect at least:
- 100+ samples for latency
- 50+ samples for accuracy
- 10+ samples for trip response

This provides statistical significance for your analysis.
