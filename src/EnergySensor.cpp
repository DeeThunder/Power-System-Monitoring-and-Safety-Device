#include "EnergySensor.h"
#include "config.h"

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
        #ifdef DEBUG_SERIAL
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
        voltage_ = readVoltageSimulation();
        current_ = readCurrentSimulation();
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
    uint16_t rawADC = readADC(PIN_VOLTAGE_SENSOR);
    float calibrated = applyCalibration(rawADC, VOLTAGE_SLOPE, VOLTAGE_INTERCEPT);
    
    // For simulation: map ADC range to realistic voltage range (180V - 250V)
    // This allows testing with potentiometer
    float simulatedVoltage = map(rawADC, 0, ADC_RESOLUTION, 180, 250);
    
    #ifdef DEBUG_SERIAL
        // Serial.printf("[EnergySensor] Voltage ADC: %d -> %.2fV (simulated)\n", 
        //               rawADC, simulatedVoltage);
    #endif
    
    return simulatedVoltage;
}

float EnergySensor::readCurrentSimulation() {
    uint16_t rawADC = readADC(PIN_CURRENT_SENSOR);
    float calibrated = applyCalibration(rawADC, CURRENT_SLOPE, CURRENT_INTERCEPT);
    
    // For simulation: map ADC range to realistic current range (0A - 25A)
    float simulatedCurrent = map(rawADC, 0, ADC_RESOLUTION, 0, 25);
    
    #ifdef DEBUG_SERIAL
        // Serial.printf("[EnergySensor] Current ADC: %d -> %.2fA (simulated)\n", 
        //               rawADC, simulatedCurrent);
    #endif
    
    return simulatedCurrent;
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
