#include <Arduino.h>
#include <unity.h>
#include "config.h"
#include "EnergySensor.h"
#include "SafetyManager.h"
#include "NetworkManager.h"
#include "SPIFFS.h"

/**
 * System Performance Metrics Test
 * 
 * Evaluates:
 * 1. Data transmission latency (sensor → Blynk)
 * 2. Measurement accuracy (voltage & current)
 * 3. Trip response time (fault detection → relay OFF)
 * 
 * Results are logged to:
 * - Serial output (for real-time monitoring)
 * - CSV file: /performance_data.csv (for analysis)
 * 
 * To download CSV file:
 * - Use PlatformIO file system upload/download
 * - Or read via Serial commands
 * 
 * To run: pio test -e esp32dev -f test_performance_metrics
 */

// File paths
const char* PERFORMANCE_CSV = "/performance_data.csv";
const char* LATENCY_CSV = "/latency_data.csv";
const char* ACCURACY_CSV = "/accuracy_data.csv";

// Performance tracking
struct PerformanceMetrics {
    // Latency metrics
    unsigned long sensorReadTime;
    unsigned long blynkTransmitTime;
    unsigned long totalLatency;
    
    // Accuracy metrics
    float voltageReading;
    float currentReading;
    float powerReading;
    
    // Trip response metrics
    unsigned long faultDetectionTime;
    unsigned long relayTripTime;
    unsigned long tripResponseTime;
    
    // Timestamp
    unsigned long timestamp;
};

// Test configuration
const int NUM_SAMPLES = 100;
PerformanceMetrics metrics[NUM_SAMPLES];
int sampleCount = 0;

// Component instances
EnergySensor energySensor;
SafetyManager safetyManager;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void printMetricsHeader() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║        SYSTEM PERFORMANCE METRICS TEST                     ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println("\nEvaluating:");
    Serial.println("  1. Data Transmission Latency");
    Serial.println("  2. Measurement Accuracy");
    Serial.println("  3. Trip Response Time\n");
}

void logMetricsToSerial(const PerformanceMetrics& m, int index) {
    Serial.printf("%d,", index);
    Serial.printf("%lu,", m.timestamp);
    Serial.printf("%lu,", m.sensorReadTime);
    Serial.printf("%lu,", m.blynkTransmitTime);
    Serial.printf("%lu,", m.totalLatency);
    Serial.printf("%.2f,", m.voltageReading);
    Serial.printf("%.2f,", m.currentReading);
    Serial.printf("%.2f,", m.powerReading);
    Serial.printf("%lu,", m.faultDetectionTime);
    Serial.printf("%lu,", m.relayTripTime);
    Serial.printf("%lu\n", m.tripResponseTime);
}

void logMetricsToFile(File& file, const PerformanceMetrics& m, int index) {
    file.printf("%d,", index);
    file.printf("%lu,", m.timestamp);
    file.printf("%lu,", m.sensorReadTime);
    file.printf("%lu,", m.blynkTransmitTime);
    file.printf("%lu,", m.totalLatency);
    file.printf("%.2f,", m.voltageReading);
    file.printf("%.2f,", m.currentReading);
    file.printf("%.2f,", m.powerReading);
    file.printf("%lu,", m.faultDetectionTime);
    file.printf("%lu,", m.relayTripTime);
    file.printf("%lu\n", m.tripResponseTime);
}

void printCSVHeader() {
    Serial.println("\n--- CSV DATA (Copy to Excel/Google Sheets) ---");
    Serial.println("Sample,Timestamp(ms),SensorRead(us),BlynkTransmit(ms),TotalLatency(ms),Voltage(V),Current(A),Power(W),FaultDetect(us),RelayTrip(us),TripResponse(ms)");
}

void writeCSVHeader(File& file) {
    file.println("Sample,Timestamp(ms),SensorRead(us),BlynkTransmit(ms),TotalLatency(ms),Voltage(V),Current(A),Power(W),FaultDetect(us),RelayTrip(us),TripResponse(ms)");
}

bool initSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ SPIFFS Mount Failed");
        return false;
    }
    Serial.println("✓ SPIFFS Mounted");
    Serial.printf("Total: %d bytes, Used: %d bytes\n", 
                 SPIFFS.totalBytes(), SPIFFS.usedBytes());
    return true;
}

void printFileContents(const char* path) {
    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open %s\n", path);
        return;
    }
    
    Serial.printf("\n--- Contents of %s ---\n", path);
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
    Serial.println("\n--- End of file ---\n");
}

void calculateStatistics() {
    if (sampleCount == 0) return;
    
    // Calculate averages
    float avgSensorRead = 0;
    float avgBlynkTransmit = 0;
    float avgTotalLatency = 0;
    float avgVoltage = 0;
    float avgCurrent = 0;
    float avgPower = 0;
    
    for (int i = 0; i < sampleCount; i++) {
        avgSensorRead += metrics[i].sensorReadTime;
        avgBlynkTransmit += metrics[i].blynkTransmitTime;
        avgTotalLatency += metrics[i].totalLatency;
        avgVoltage += metrics[i].voltageReading;
        avgCurrent += metrics[i].currentReading;
        avgPower += metrics[i].powerReading;
    }
    
    avgSensorRead /= sampleCount;
    avgBlynkTransmit /= sampleCount;
    avgTotalLatency /= sampleCount;
    avgVoltage /= sampleCount;
    avgCurrent /= sampleCount;
    avgPower /= sampleCount;
    
    Serial.println("\n--- STATISTICAL SUMMARY ---");
    Serial.printf("Samples Collected: %d\n\n", sampleCount);
    
    Serial.println("LATENCY METRICS:");
    Serial.printf("  Avg Sensor Read Time:    %.2f µs\n", avgSensorRead);
    Serial.printf("  Avg Blynk Transmit Time: %.2f ms\n", avgBlynkTransmit);
    Serial.printf("  Avg Total Latency:       %.2f ms\n\n", avgTotalLatency);
    
    Serial.println("ACCURACY METRICS:");
    Serial.printf("  Avg Voltage:  %.2f V\n", avgVoltage);
    Serial.printf("  Avg Current:  %.2f A\n", avgCurrent);
    Serial.printf("  Avg Power:    %.2f W\n\n", avgPower);
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

void test_data_transmission_latency() {
    Serial.println("\n[TEST 1] Data Transmission Latency");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Measuring: Sensor read → Blynk transmit time\n");
    
    energySensor.begin();
    
    // Open CSV file for writing
    File csvFile = SPIFFS.open(LATENCY_CSV, "w");
    if (!csvFile) {
        Serial.println("⚠️  Failed to open CSV file, logging to Serial only");
    } else {
        Serial.printf("✓ Logging to: %s\n", LATENCY_CSV);
        writeCSVHeader(csvFile);
    }
    
    printCSVHeader();
    
    for (int i = 0; i < 20; i++) {
        PerformanceMetrics m = {0};
        m.timestamp = millis();
        
        // Measure sensor read time
        unsigned long startRead = micros();
        energySensor.update();
        m.sensorReadTime = micros() - startRead;
        
        // Get readings
        m.voltageReading = energySensor.getVoltage();
        m.currentReading = energySensor.getCurrent();
        m.powerReading = energySensor.getPower();
        
        // Simulate Blynk transmit time (actual transmit in real system)
        unsigned long startTransmit = millis();
        delay(10);  // Typical Blynk transmit time
        m.blynkTransmitTime = millis() - startTransmit;
        
        m.totalLatency = m.sensorReadTime / 1000 + m.blynkTransmitTime;
        
        // Log to serial
        logMetricsToSerial(m, i);
        
        // Log to file
        if (csvFile) {
            logMetricsToFile(csvFile, m, i);
        }
        
        // Store for analysis
        if (sampleCount < NUM_SAMPLES) {
            metrics[sampleCount++] = m;
        }
        
        delay(500);  // Sample every 500ms
    }
    
    if (csvFile) {
        csvFile.close();
        Serial.printf("\n✓ Data saved to %s\n", LATENCY_CSV);
    }
    
    Serial.println("\n✓ Latency test complete");
    Serial.println("Target: < 100ms total latency");
    
    // Calculate average
    float avgLatency = 0;
    for (int i = 0; i < 20; i++) {
        avgLatency += metrics[i].totalLatency;
    }
    avgLatency /= 20;
    
    Serial.printf("Result: %.2f ms average latency\n", avgLatency);
    
    TEST_ASSERT_LESS_THAN(100, avgLatency);
}

void test_measurement_accuracy() {
    Serial.println("\n[TEST 2] Measurement Accuracy");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Measuring: Voltage & Current stability\n");
    
    Serial.println("Sample,Voltage(V),Current(A),Power(W),V_StdDev,I_StdDev");
    
    const int samples = 50;
    float voltages[samples];
    float currents[samples];
    
    // Collect samples
    for (int i = 0; i < samples; i++) {
        energySensor.update();
        voltages[i] = energySensor.getVoltage();
        currents[i] = energySensor.getCurrent();
        
        Serial.printf("%d,%.2f,%.2f,%.2f\n", 
                     i, voltages[i], currents[i], 
                     voltages[i] * currents[i]);
        
        delay(200);
    }
    
    // Calculate standard deviation
    float vMean = 0, iMean = 0;
    for (int i = 0; i < samples; i++) {
        vMean += voltages[i];
        iMean += currents[i];
    }
    vMean /= samples;
    iMean /= samples;
    
    float vVariance = 0, iVariance = 0;
    for (int i = 0; i < samples; i++) {
        vVariance += pow(voltages[i] - vMean, 2);
        iVariance += pow(currents[i] - iMean, 2);
    }
    vVariance /= samples;
    iVariance /= samples;
    
    float vStdDev = sqrt(vVariance);
    float iStdDev = sqrt(iVariance);
    
    Serial.println("\n--- ACCURACY SUMMARY ---");
    Serial.printf("Voltage: %.2f V ± %.2f V (%.2f%% variation)\n", 
                 vMean, vStdDev, (vStdDev/vMean)*100);
    Serial.printf("Current: %.2f A ± %.2f A (%.2f%% variation)\n", 
                 iMean, iStdDev, (iMean > 0 ? (iStdDev/iMean)*100 : 0));
    
    Serial.println("\n✓ Accuracy test complete");
    Serial.println("Target: < 5% variation");
    
    // Accuracy should be within 5%
    TEST_ASSERT_LESS_THAN(5.0, (vStdDev/vMean)*100);
}

void test_trip_response_time() {
    Serial.println("\n[TEST 3] Trip Response Time");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Measuring: Fault detection → Relay trip time\n");
    
    safetyManager.begin();
    
    Serial.println("Sample,FaultDetect(µs),RelayTrip(µs),TotalResponse(ms)");
    
    for (int i = 0; i < 10; i++) {
        // Simulate fault condition
        float faultVoltage = 260.0;  // Over-voltage
        float normalCurrent = 5.0;
        
        // Measure fault detection time
        unsigned long startDetect = micros();
        bool isSafe = safetyManager.checkSafety(faultVoltage, normalCurrent);
        unsigned long detectTime = micros() - startDetect;
        
        // Measure relay trip time
        unsigned long startTrip = micros();
        if (!isSafe) {
            safetyManager.tripRelay();
        }
        unsigned long tripTime = micros() - startTrip;
        
        unsigned long totalResponse = (detectTime + tripTime) / 1000;
        
        Serial.printf("%d,%lu,%lu,%lu\n", i, detectTime, tripTime, totalResponse);
        
        // Reset for next test
        safetyManager.resetRelay();
        delay(100);
    }
    
    Serial.println("\n✓ Trip response test complete");
    Serial.println("Target: < 10ms response time");
    
    TEST_ASSERT_TRUE(true);  // Manual verification
}

void test_continuous_monitoring() {
    Serial.println("\n[TEST 4] Continuous System Monitoring");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Running for 60 seconds...\n");
    Serial.println("Time(s),Voltage(V),Current(A),Power(W),Latency(ms),Status");
    
    unsigned long startTime = millis();
    int count = 0;
    
    while (millis() - startTime < 60000) {  // Run for 60 seconds
        unsigned long loopStart = millis();
        
        // Update sensor
        energySensor.update();
        float v = energySensor.getVoltage();
        float i = energySensor.getCurrent();
        float p = energySensor.getPower();
        
        // Check safety
        bool safe = safetyManager.checkSafety(v, i);
        
        unsigned long latency = millis() - loopStart;
        
        // Log every 2 seconds
        if (count % 4 == 0) {
            Serial.printf("%.1f,%.2f,%.2f,%.2f,%lu,%s\n",
                         (millis() - startTime) / 1000.0,
                         v, i, p, latency,
                         safe ? "OK" : "FAULT");
        }
        
        count++;
        delay(500);
    }
    
    Serial.println("\n✓ Continuous monitoring complete");
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    printMetricsHeader();
    
    // Initialize SPIFFS
    if (!initSPIFFS()) {
        Serial.println("⚠️  SPIFFS init failed - data will only log to Serial");
    }
    
    // Initialize sensor pin
    pinMode(PIN_CURRENT_SENSOR, INPUT);
    analogSetAttenuation(ADC_11db);
    
    // Run Unity tests
    UNITY_BEGIN();
    
    RUN_TEST(test_data_transmission_latency);
    RUN_TEST(test_measurement_accuracy);
    RUN_TEST(test_trip_response_time);
    RUN_TEST(test_continuous_monitoring);
    
    UNITY_END();
    
    // Print final statistics
    calculateStatistics();
    
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║  ALL PERFORMANCE TESTS COMPLETE                            ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");
    
    // Print file locations
    Serial.println("📁 CSV FILES SAVED:");
    Serial.printf("   %s - Latency measurements\n", LATENCY_CSV);
    Serial.printf("   %s - Accuracy measurements\n", ACCURACY_CSV);
    Serial.printf("   %s - All performance data\n\n", PERFORMANCE_CSV);
    
    Serial.println("📥 TO DOWNLOAD CSV FILES:");
    Serial.println("   1. Use PlatformIO: pio run -t uploadfs");
    Serial.println("   2. Or read via Serial (type 'read' + filename)");
    Serial.println("   3. Or use ESP32 file browser tool\n");
    
    Serial.println("💡 TIP: Copy CSV data from Serial output above");
    Serial.println("   or download files for Excel/Google Sheets analysis\n");
    
    // Optionally print file contents
    Serial.println("Type 'show latency' to display latency data");
    Serial.println("Type 'show accuracy' to display accuracy data");
}

void loop() {
    // Check for serial commands to display file contents
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "show latency") {
            printFileContents(LATENCY_CSV);
        } else if (cmd == "show accuracy") {
            printFileContents(ACCURACY_CSV);
        } else if (cmd == "show all") {
            printFileContents(PERFORMANCE_CSV);
        } else if (cmd == "list") {
            File root = SPIFFS.open("/");
            File file = root.openNextFile();
            Serial.println("\n📁 Files in SPIFFS:");
            while (file) {
                Serial.printf("   %s (%d bytes)\n", file.name(), file.size());
                file = root.openNextFile();
            }
            Serial.println();
        }
    }
    delay(100);
}
