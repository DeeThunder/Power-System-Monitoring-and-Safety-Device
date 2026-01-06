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
    
    // Set ADC resolution to 12-bit for better precision
    analogReadResolution(12);
    
    // Set ADC attenuation for 0-3.3V range
    analogSetAttenuation(ADC_11db);  // 0-3.3V range
    
    #ifdef SIMULATION_MODE
        simulationMode_ = true;
        #ifdef APP_DEBUG
            Serial.println("[EnergySensor] Initialized in SIMULATION mode");
        #endif
    #else
        simulationMode_ = false;
        #ifdef APP_DEBUG
            Serial.println("[EnergySensor] Initialized in PRODUCTION mode with EmonLib");
            Serial.printf("[EnergySensor] Current calibration factor: %.1f\n", CURRENT_CALIBRATION_FACTOR);
        #endif
        // Initialize EmonLib for current sensing
        // SCT-013-100: 100A/1V sensor with voltage divider
        emon1_.current(PIN_CURRENT_SENSOR, CURRENT_CALIBRATION_FACTOR);
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
            
            // If current drops to 0 (below noise threshold), reset immediately
            // Otherwise apply smoothing
            if (rawCurrent == 0.0) {
                current_ = 0.0;
            } else {
                current_ = (alpha * rawCurrent) + ((1.0 - alpha) * current_);
            }
        }
        
    } else {
        #ifndef SIMULATION_MODE
            // Production mode: Use EmonLib for current, simulation for voltage
            voltage_ = readVoltageSimulation();
            
            // Take multiple samples and average for stability
            double sum = 0;
            for (int i = 0; i < CURRENT_NUM_AVERAGES; i++) {
                double Irms = emon1_.calcIrms(CURRENT_EMON_SAMPLES);
                sum += Irms;
                delay(100); // Small delay between samples
            }
            double avgCurrent = sum / CURRENT_NUM_AVERAGES;
            
            // Basic noise floor removal
            if (avgCurrent < CURRENT_NOISE_THRESHOLD) {
                avgCurrent = 0;
            }
            
            current_ = avgCurrent;
            
            #ifdef APP_DEBUG
                Serial.printf("[EnergySensor] EmonLib Current: %.2f A\n", current_);
            #endif
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
    // Current sensor (SCT-013-100) AC reading with RMS calculation
    // Enhanced EMI filtering with more samples and better settling time
    const int numSamples = 1000;  // Increased for better noise rejection
    const float adcVoltageRef = 3.3;
    float sum = 0.0;
    float sumSquares = 0.0;
    
    // Two-pass algorithm for accurate DC bias removal
    // Pass 1: Calculate Mean (DC Bias) with ADC settling
    long biasSum = 0;
    
    // Discard first few samples to allow ADC to settle
    for (int i = 0; i < 10; i++) {
        analogRead(PIN_CURRENT_SENSOR);
        delayMicroseconds(50);
    }
    
    for (int i = 0; i < numSamples; i++) {
        biasSum += analogRead(PIN_CURRENT_SENSOR);
        delayMicroseconds(200);  // Longer delay for better AC cycle coverage
    }
    float dcBias = biasSum / (float)numSamples;
    
    // Pass 2: Calculate RMS relative to DC Bias
    for (int i = 0; i < numSamples; i++) {
        float sample = analogRead(PIN_CURRENT_SENSOR);
        float acValue = sample - dcBias;
        sumSquares += (acValue * acValue);
        delayMicroseconds(200);  // Match Pass 1 timing
    }
    
    // Convert to RMS ADC coordinates
    float rmsADC = sqrt(sumSquares / numSamples);
    
    // Convert to Voltage (at ADC input)
    float rmsVoltage = (rmsADC / (float)ADC_RESOLUTION) * adcVoltageRef;
    
    // EMI Rejection: If RMS voltage is extremely low (< 1mV), it's likely EMI noise
    // Real current flow produces higher RMS voltages
    if (rmsVoltage < 0.001) {  // Less than 1mV = EMI noise
        #ifdef APP_DEBUG
            Serial.printf("[EnergySensor] EMI detected - RMS too low: %.6fV\n", rmsVoltage);
        #endif
        return 0.0;
    }
    
    // Convert RMS voltage to Current using calibration factor
    // SCT-013-100: 1V RMS output @ 100A max current
    // Formula: Current (A) = RMS Voltage (V) × Calibration Factor
    float rawCurrent = rmsVoltage * CURRENT_CALIBRATION_FACTOR;
    
    // Apply calibration correction for DC offset and scaling
    // Calibrated using two points (fresh raw readings):
    // - Raw 0.19A → Actual 0.05A (clamp meter, low current)
    // - Raw 0.20A → Actual 0.41A (clamp meter, high current)
    // 
    // Linear regression: Actual = m × Raw + b
    // Slope (m) = (0.41 - 0.05) / (0.20 - 0.19) = 36.0
    // Intercept (b) = 0.05 - (36.0 × 0.19) = -6.79
    // 
    // Formula: Actual = (Raw × 36.0) - 6.79
    float current = (rawCurrent * 36.0) - 6.79;
    
    // Prevent negative readings from offset
    if (current < 0) {
        current = 0.0;
    }
    
    // Noise gate - filter out readings below threshold
    if (current < CURRENT_NOISE_THRESHOLD) {
        current = 0.0;
    }
    
    #ifdef APP_DEBUG
        Serial.printf("[EnergySensor] Current - DC Bias: %.2f, RMS ADC: %.2f, RMS V: %.4fV, Raw I: %.3fA, Corrected I: %.3fA\n", 
                      dcBias, rmsADC, rmsVoltage, rawCurrent, current);
        Serial.printf("[EnergySensor] Current - Final: %.3fA (After noise gate: %.2fA threshold)\n", 
                      current, CURRENT_NOISE_THRESHOLD);
    #endif
    
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
