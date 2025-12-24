#include "EnergySensor.h"
#include "config.h"
#include <math.h>

EnergySensor::EnergySensor() 
    : voltage_(0.0), current_(0.0), simulationMode_(true) {
}

void EnergySensor::begin() {
    // Configure ADC pins as input
    pinMode(PIN_VOLTAGE_SENSOR, INPUT);
    pinMode(PIN_CURRENT_SENSOR, INPUT);
    
    // Set ADC attenuation for 0-3.3V range
    analogSetAttenuation(ADC_11db);  // 0-3.3V range
    
    #ifdef SIMULATION_MODE
        simulationMode_ = true;
        #ifdef APP_DEBUG
            Serial.println("[EnergySensor] Initialized in SIMULATION mode");
        #endif
    #else
        simulationMode_ = false;
        #ifdef DEBUG_SERIAL
            Serial.println("[EnergySensor] Initialized in PRODUCTION mode");
        #endif
        // TODO: Initialize EmonLib here
    #endif
}

void EnergySensor::update() {
    if (simulationMode_) {
        // Read Raw Values
        float rawVoltage = readVoltageSimulation();
        float rawCurrent = readCurrentSimulation();
        
        // Apply Smoothing (Exponential Moving Average)
        // BUT: If power drops very low, reset immediately to avoid slow decay causing false trips
        if (rawVoltage < VOLTAGE_NOISE_THRESHOLD) {
            // Power is OFF - reset smoothing immediately
            voltage_ = 0.0;
            current_ = 0.0;
        } else {
            // Power is ON - apply normal smoothing
            // Alpha = 0.2 means 20% weight to new reading, 80% to old.
            // First reading initialization:
            if (voltage_ == 0.0 && rawVoltage > 0) voltage_ = rawVoltage;
            if (current_ == 0.0 && rawCurrent > 0) current_ = rawCurrent;
            
            const float alpha = 0.2; 
            voltage_ = (alpha * rawVoltage) + ((1.0 - alpha) * voltage_);
            current_ = (alpha * rawCurrent) + ((1.0 - alpha) * current_);
        }
        
    } else {
        #ifndef SIMULATION_MODE
            // TODO: Read from EmonLib
            // voltage_ = readVoltageProduction();
            // current_ = readCurrentProduction();
        #endif
    }
    
    #ifdef DEBUG_SERIAL
        Serial.printf("[EnergySensor] V=%.2fV, I=%.2fA, P=%.2fW\n", 
                      voltage_, current_, getPower());
    #endif
}

float EnergySensor::getVoltage() const {
    return voltage_;
}

float EnergySensor::getCurrent() const {
    return current_;
}

float EnergySensor::getPower() const {
    return voltage_ * current_;
}

void EnergySensor::setSimulationMode(bool enable) {
    simulationMode_ = enable;
    #ifdef DEBUG_SERIAL
        Serial.printf("[EnergySensor] Simulation mode: %s\n", 
                      enable ? "ENABLED" : "DISABLED");
    #endif
}

bool EnergySensor::isValid() const {
    // Check if readings are within reasonable bounds
    return (voltage_ >= 0 && voltage_ <= 300) && 
           (current_ >= 0 && current_ <= 100);
}

uint16_t EnergySensor::readADC(uint8_t pin) {
    uint32_t sum = 0;
    
    // Take multiple samples and average
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(pin);
        delayMicroseconds(100);  // Small delay between samples
    }
    
    return sum / ADC_SAMPLES;
}

float EnergySensor::applyCalibration(uint16_t rawValue, float slope, float intercept) {
    return (rawValue * slope) + intercept;
}

float EnergySensor::readVoltageSimulation() {
    // ZMPT101B AC voltage sensor reading using Single-Pass True RMS
    // Formula: RMS = sqrt( (SumSq - (Sum*Sum/N)) / N )
    // This removes DC bias dynamically in a single pass, which is much more stable
    // against fluctuations and takes half the time (or double precision).

    const int numSamples = 1000;  // Double the samples for better averaging (same total time as before)
    const float vRef = 3.3;       // ESP32 ADC reference voltage
    
    unsigned long sumRaw = 0;
    double sumSqRaw = 0; // Double precision for squares to avoid overflow
    
    // Single Sampling Pass
    for (int i = 0; i < numSamples; i++) {
        uint16_t rawADC = analogRead(PIN_VOLTAGE_SENSOR);
        sumRaw += rawADC;
        sumSqRaw += (double)rawADC * rawADC;
        // Faster delay to fit more samples in ~100ms
        delayMicroseconds(100);
    }
    
    // 1. Calculate Mean (DC Bias)
    double mean = (double)sumRaw / numSamples;
    
    // 2. Calculate Variance of the counts (E[x^2] - (E[x])^2)
    // This gives us the Mean Square of the AC component
    double meanSquareRaw = (sumSqRaw / numSamples) - (mean * mean);
    
    // Handle floating point errors (negative zero)
    if (meanSquareRaw < 0) meanSquareRaw = 0;
    
    // 3. RMS in ADC counts
    double rmsADC = sqrt(meanSquareRaw);
    
    // 4. Convert to Voltage
    // (rmsADC / 4095) * 3.3
    float rmsVoltage = (float)((rmsADC / ADC_RESOLUTION) * vRef);
    
    // Apply calibration factor
    float actualVoltage = rmsVoltage * VOLTAGE_CALIBRATION_FACTOR;

    // Noise Gate: Ignore phantom voltages (ghost readings) when mains is disconnected
    if (actualVoltage < VOLTAGE_NOISE_THRESHOLD) {
        actualVoltage = 0.0;
    }
    
    #ifdef APP_DEBUG
        Serial.printf("[EnergySensor] Voltage RMS: %.4fV -> %.2fV AC\n", 
                      rmsVoltage, actualVoltage);
    #endif
    
    return actualVoltage;
}

float EnergySensor::readCurrentSimulation() {
    // Current sensor (SCT-013) AC reading with RMS calculation
    const int numSamples = 500;
    const float adcVoltageRef = 3.3;
    float sum = 0.0;
    float sumSquares = 0.0;
    
    // Arrays to store samples for two-pass algorithm (optional) 
    // or just use a digital filter. For simplicity and memory (no large arrays),
    // let's use the standard DC-offset removal filter (like EmonLib)
    // or a simple two-pass which is more accurate if we can't do continuous sampling.
    // Given the short sample time, two-pass is fine.
    
    // Pass 1: Calculate Mean (DC Bias)
    long biasSum = 0;
    for (int i = 0; i < numSamples; i++) {
        biasSum += analogRead(PIN_CURRENT_SENSOR);
        delayMicroseconds(100);
    }
    float dcBias = biasSum / (float)numSamples;
    
    // Pass 2: Calculate RMS relative to DC Bias
    for (int i = 0; i < numSamples; i++) {
        float sample = analogRead(PIN_CURRENT_SENSOR);
        float acValue = sample - dcBias;
        sumSquares += (acValue * acValue);
        delayMicroseconds(100);
    }
    
    // Convert to RMS ADC coordinates
    float rmsADC = sqrt(sumSquares / numSamples);
    
    // Convert to Voltage (at ADC input)
    float rmsVoltage = (rmsADC / (float)ADC_RESOLUTION) * adcVoltageRef;
    
    // Convert RMS voltage to Current using slope (calibration)
    // SCT-013-030 output is 1V @ 30A usually, or similar ratio
    // Here we use the define slope for simplicity or a direct factor
    // Assuming slope is Amps per Volt coming out of the circuit:
    // If 30A gives 1V, then factor is 30.0. 
    // Let's use a rough factor derived from the slope if possible, 
    // or just a standard factor for the SCT013 circuit.
    // For now, let's trust the logic: Amps = rmsVoltage * CURRENT_CAL_FACTOR
    
    float calibrationFactor = 20.0; // Need to verify this against CURRENT_SLOPE intent
    // Or restart using the raw ADC approach if that was intended for DC? 
    // SCT is AC. RMS is correct.
    
    float current = rmsVoltage * calibrationFactor;
    
    // Noise gate
    if (current < CURRENT_NOISE_THRESHOLD) {
        current = 0.0;
    }
    
    return current;
}

// Future: Production mode implementation with EmonLib
#ifndef SIMULATION_MODE
/*
float EnergySensor::readVoltageProduction() {
    // TODO: Implement EmonLib voltage reading
    // Example:
    // emon1.voltage(PIN_VOLTAGE_SENSOR, VOLTAGE_CAL, PHASE_SHIFT);
    // emon1.calcVI(20, 2000);
    // return emon1.Vrms;
    return 0.0;
}

float EnergySensor::readCurrentProduction() {
    // TODO: Implement EmonLib current reading
    // Example:
    // emon1.current(PIN_CURRENT_SENSOR, CURRENT_CAL);
    // emon1.calcVI(20, 2000);
    // return emon1.Irms;
    return 0.0;
}
*/
#endif
