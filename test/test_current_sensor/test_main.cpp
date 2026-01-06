#include <Arduino.h>
#include <unity.h>
#include "config.h"

/**
 * SCT-013 Current Sensor Diagnostic Test
 * 
 * This test helps diagnose and calibrate the SCT-013-100 current sensor.
 * 
 * To run this test:
 * 1. pio test -e esp32dev -f test_current_sensor
 * 2. Or upload and run manually
 */

// Test configuration
const int NUM_SAMPLES = 1000;
const float ADC_VOLTAGE_REF = 3.3;

// Relay control helpers
void relayON() {
    Serial.println("\n>>> RELAY ON COMMAND SENT <<<");
    Serial.printf("Setting GPIO%d to %s\n", PIN_RELAY, RELAY_ACTIVE_HIGH ? "HIGH" : "LOW");
    
    #if RELAY_ACTIVE_HIGH
        digitalWrite(PIN_RELAY, HIGH);
    #else
        digitalWrite(PIN_RELAY, LOW);
    #endif
    
    delay(100);  // Give relay time to energize
    
    int state = digitalRead(PIN_RELAY);
    Serial.printf("GPIO%d readback: %s\n", PIN_RELAY, state == HIGH ? "HIGH" : "LOW");
    Serial.println("✓ Relay turned ON - Current should flow");
    Serial.println(">>> If relay clicked OFF, ESP32 may have reset <<<\n");
}

void relayOFF() {
    Serial.println("\n>>> RELAY OFF COMMAND SENT <<<");
    Serial.printf("Setting GPIO%d to %s\n", PIN_RELAY, RELAY_ACTIVE_HIGH ? "LOW" : "HIGH");
    
    #if RELAY_ACTIVE_HIGH
        digitalWrite(PIN_RELAY, LOW);
    #else
        digitalWrite(PIN_RELAY, HIGH);
    #endif
    
    delay(100);
    
    int state = digitalRead(PIN_RELAY);
    Serial.printf("GPIO%d readback: %s\n", PIN_RELAY, state == HIGH ? "HIGH" : "LOW");
    Serial.println("✓ Relay turned OFF - No current flow\n");
}

// Test results storage
struct SensorReading {
    float dcBias;
    uint16_t minADC;
    uint16_t maxADC;
    float rmsADC;
    float rmsVoltage;
    float peakToPeak;
};

SensorReading readCurrentSensor() {
    SensorReading result = {0};
    
    // Pass 1: Calculate DC Bias
    long biasSum = 0;
    result.minADC = 4095;
    result.maxADC = 0;
    
    // Discard first samples for ADC settling
    for (int i = 0; i < 10; i++) {
        analogRead(PIN_CURRENT_SENSOR);
        delayMicroseconds(50);
    }
    
    // Collect DC bias samples
    for (int i = 0; i < NUM_SAMPLES; i++) {
        uint16_t sample = analogRead(PIN_CURRENT_SENSOR);
        biasSum += sample;
        if (sample < result.minADC) result.minADC = sample;
        if (sample > result.maxADC) result.maxADC = sample;
        delayMicroseconds(200);
    }
    
    result.dcBias = biasSum / (float)NUM_SAMPLES;
    
    // Pass 2: Calculate RMS
    double sumSquares = 0.0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        float sample = analogRead(PIN_CURRENT_SENSOR);
        float acValue = sample - result.dcBias;
        sumSquares += (acValue * acValue);
        delayMicroseconds(200);
    }
    
    result.rmsADC = sqrt(sumSquares / NUM_SAMPLES);
    result.rmsVoltage = (result.rmsADC / 4095.0) * ADC_VOLTAGE_REF;
    result.peakToPeak = ((result.maxADC - result.minADC) / 4095.0) * ADC_VOLTAGE_REF;
    
    return result;
}

void printSensorReading(const SensorReading& reading) {
    Serial.println("\n═══════════════════════════════════════════════════════");
    Serial.printf("DC Bias:       %.2f counts (%.3fV)\n", 
                  reading.dcBias, (reading.dcBias / 4095.0) * ADC_VOLTAGE_REF);
    Serial.printf("Min ADC:       %d counts\n", reading.minADC);
    Serial.printf("Max ADC:       %d counts\n", reading.maxADC);
    Serial.printf("Peak-to-Peak:  %.2fmV\n", reading.peakToPeak * 1000);
    Serial.printf("RMS ADC:       %.2f counts\n", reading.rmsADC);
    Serial.printf("RMS Voltage:   %.4fV (%.2fmV)\n", 
                  reading.rmsVoltage, reading.rmsVoltage * 1000);
    Serial.println("═══════════════════════════════════════════════════════");
}

void printCalibrationGuide(float rmsVoltage) {
    if (rmsVoltage > 0.005) {
        Serial.println("\n📐 CALIBRATION FACTORS:");
        Serial.printf("   1A  → CAL = %.0f\n", 1.0 / rmsVoltage);
        Serial.printf("   2A  → CAL = %.0f\n", 2.0 / rmsVoltage);
        Serial.printf("   5A  → CAL = %.0f\n", 5.0 / rmsVoltage);
        Serial.printf("   10A → CAL = %.0f\n", 10.0 / rmsVoltage);
        Serial.println("\nUse your clamp meter reading to pick the right factor!");
    }
}

// ============================================================================
// UNITY TESTS
// ============================================================================

void test_dc_bias_centered() {
    SensorReading reading = readCurrentSensor();
    float dcBiasVoltage = (reading.dcBias / 4095.0) * ADC_VOLTAGE_REF;
    
    Serial.printf("\n[TEST] DC Bias: %.3fV\n", dcBiasVoltage);
    
    // DC bias should be around 1.65V (±0.2V tolerance)
    TEST_ASSERT_FLOAT_WITHIN(0.2, 1.65, dcBiasVoltage);
}

void test_noise_floor_no_load() {
    Serial.println("\n[TEST] Checking noise floor (no load expected)...");
    
    // Turn relay OFF to ensure no current flows
    relayOFF();
    delay(1000);  // Wait for relay to settle
    
    SensorReading reading = readCurrentSensor();
    printSensorReading(reading);
    
    // RMS voltage should be < 5mV with no load (ideal)
    Serial.printf("\n[TEST] RMS Voltage: %.2fmV\n", reading.rmsVoltage * 1000);
    Serial.println("Target: < 5mV (ideal), < 50mV (acceptable with some EMI)");
    
    if (reading.rmsVoltage > 0.050) {
        Serial.println("\n⚠️  HIGH NOISE DETECTED!");
        Serial.printf("   Current: %.2fmV (%.1fx over target)\n", 
                     reading.rmsVoltage * 1000, 
                     reading.rmsVoltage / 0.005);
        Serial.println("\n   Troubleshooting steps:");
        Serial.println("   1. Check ZMPT101B distance (need >20cm)");
        Serial.println("   2. Try perpendicular orientation (90° angle)");
        Serial.println("   3. Disconnect ZMPT101B from mains temporarily");
        Serial.println("   4. Check for ground loops");
        Serial.println("   5. Add magnetic shielding (aluminum foil)");
    } else if (reading.rmsVoltage > 0.005) {
        Serial.println("\n⚠️  Moderate noise detected");
        Serial.println("   Acceptable for operation, but calibration may be affected");
    } else {
        Serial.println("\n✓ EXCELLENT! Very low noise floor");
    }
    
    // Make test informational - always pass but show the reading
    TEST_ASSERT_TRUE_MESSAGE(true, 
        "Noise floor test is informational - check readings above");
}

void test_current_reading_with_load() {
    Serial.println("\n[TEST] Reading current with load...");
    Serial.println("MANUAL STEP: Ensure a load is connected to the socket!");
    
    // Turn relay ON to allow current to flow
    relayON();
    Serial.println("Waiting 3 seconds for relay to settle...\n");
    delay(3000);
    
    SensorReading reading = readCurrentSensor();
    printSensorReading(reading);
    printCalibrationGuide(reading.rmsVoltage);
    
    // With load, RMS should be > 5mV
    Serial.printf("\n[TEST] RMS Voltage: %.2fmV\n", reading.rmsVoltage * 1000);
    
    if (reading.rmsVoltage > 0.005) {
        float estimatedCurrent = reading.rmsVoltage * CURRENT_CALIBRATION_FACTOR;
        Serial.printf("✓ Estimated Current: %.2fA (CAL=%.0f)\n", 
                     estimatedCurrent, (float)CURRENT_CALIBRATION_FACTOR);
        Serial.println("✓ Load detected - current is flowing!");
    } else {
        Serial.println("⚠️  No current detected");
        Serial.println("   Possible reasons:");
        Serial.println("   - No load connected to socket");
        Serial.println("   - Load is turned off");
        Serial.println("   - Wire not through SCT-013 sensor");
        Serial.println("   - Relay not working");
    }
    
    // Turn relay OFF after test
    relayOFF();
    
    // Always pass - this is informational only
    // User needs to manually verify with a load connected
    TEST_ASSERT_TRUE_MESSAGE(true, 
        "This test is informational - connect a load to see current readings");
}

void test_zmpt101b_interference() {
    Serial.println("\n[TEST] ZMPT101B Interference Test");
    Serial.println("═══════════════════════════════════════");
    Serial.println("\nStep 1: Reading with ZMPT101B connected...");
    
    SensorReading reading1 = readCurrentSensor();
    Serial.printf("RMS Voltage: %.2fmV\n", reading1.rmsVoltage * 1000);
    
    Serial.println("\n⚠️  MANUAL STEP: DISCONNECT ZMPT101B from mains!");
    Serial.println("Press RESET when ready...");
    
    while(true) {
        delay(1000);
        // Wait for reset
    }
}

void test_calibration_interactive() {
    Serial.println("\n[TEST] Interactive Calibration");
    Serial.println("═══════════════════════════════════════");
    Serial.println("\nThis test runs for 60 seconds for calibration.");
    Serial.println("Connect different loads and note the readings.");
    Serial.println("Relay will be ON to allow current flow.\n");
    
    // Turn relay ON for calibration
    relayON();
    delay(2000);
    
    unsigned long startTime = millis();
    unsigned long testDuration = 60000;  // 60 seconds
    int count = 0;
    
    Serial.println(">>> CALIBRATION MODE - RELAY SHOULD STAY ON <<<\n");
    
    while(millis() - startTime < testDuration) {
        count++;
        
        // Keep relay ON (re-assert every loop to prevent it turning off)
        #if RELAY_ACTIVE_HIGH
            digitalWrite(PIN_RELAY, HIGH);
        #else
            digitalWrite(PIN_RELAY, LOW);
        #endif
        
        Serial.printf("\n--- Reading #%d (%.0fs remaining) ---\n", 
                     count, (testDuration - (millis() - startTime)) / 1000.0);
        
        SensorReading reading = readCurrentSensor();
        Serial.printf("⏱ Time: %lu ms | RMS: %.2fmV | Estimated: %.2fA\n",
                     millis(), 
                     reading.rmsVoltage * 1000,
                     reading.rmsVoltage * CURRENT_CALIBRATION_FACTOR);
        
        if (reading.rmsVoltage > 0.005) {
            printCalibrationGuide(reading.rmsVoltage);
        }
        
        delay(5000);
    }
    
    Serial.println("\n>>> CALIBRATION TEST COMPLETE <<<");
    Serial.println("Relay will turn OFF now.\n");
    
    // Turn relay OFF after calibration
    relayOFF();
    
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  SCT-013 CURRENT SENSOR TEST SUITE    ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // Initialize relay pin
    pinMode(PIN_RELAY, OUTPUT);
    relayOFF();  // Start with relay OFF
    
    // Initialize sensor pin
    pinMode(PIN_CURRENT_SENSOR, INPUT);
    analogSetAttenuation(ADC_11db);
    
    Serial.println("\nSensor: SCT-013-100 (100A/1V)");
    Serial.println("Pin: GPIO35 (ADC1_CH7)");
    Serial.printf("Relay: GPIO%d\n", PIN_RELAY);
    Serial.printf("Current CAL_FACTOR: %.0f\n\n", (float)CURRENT_CALIBRATION_FACTOR);
    
    // Run Unity tests
    UNITY_BEGIN();
    
    RUN_TEST(test_dc_bias_centered);
    RUN_TEST(test_noise_floor_no_load);
    RUN_TEST(test_current_reading_with_load);
    RUN_TEST(test_calibration_interactive);
    // Uncomment to test ZMPT101B interference:
    // RUN_TEST(test_zmpt101b_interference);
    UNITY_END();
}

void loop() {
    // Everything runs in setup()
}
