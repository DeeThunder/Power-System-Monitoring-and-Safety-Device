#include "SafetyManager.h"
#include "config.h"

SafetyManager::SafetyManager() 
    : relayTripped_(false), lastFaultReason_(""), 
      lastVoltage_(0.0), lastCurrent_(0.0) {
}

void SafetyManager::begin() {
    // Configure relay pin
    pinMode(PIN_RELAY, OUTPUT);
    resetRelay();  // Start with relay in safe state (de-energized)
    
    // Configure RGB LED pins
    pinMode(PIN_RGB_RED, OUTPUT);
    pinMode(PIN_RGB_GREEN, OUTPUT);
    pinMode(PIN_RGB_BLUE, OUTPUT);
    
    // Set initial RGB color (Blue = Initializing)
    setRGBStatus(RGB_BLUE);
    
    #ifdef DEBUG_SERIAL
        Serial.println("[SafetyManager] Initialized");
        Serial.printf("[SafetyManager] Relay mode: %s\n", 
                      RELAY_ACTIVE_HIGH ? "Active HIGH" : "Active LOW");
    #endif
}

bool SafetyManager::checkSafety(float voltage, float current) {
    lastVoltage_ = voltage;
    lastCurrent_ = current;
    
    // Check over-voltage
    if (checkOverVoltage(voltage)) {
        lastFaultReason_ = "OVER VOLTAGE";
        #ifdef DEBUG_SERIAL
            Serial.printf("[SafetyManager] FAULT: Over-voltage detected (%.2fV)\n", voltage);
        #endif
        return false;
    }
    
    // Check under-voltage
    if (checkUnderVoltage(voltage)) {
        lastFaultReason_ = "UNDER VOLTAGE";
        #ifdef DEBUG_SERIAL
            Serial.printf("[SafetyManager] FAULT: Under-voltage detected (%.2fV)\n", voltage);
        #endif
        return false;
    }
    
    // Check over-current
    if (checkOverCurrent(current)) {
        lastFaultReason_ = "OVER CURRENT";
        #ifdef DEBUG_SERIAL
            Serial.printf("[SafetyManager] FAULT: Over-current detected (%.2fA)\n", current);
        #endif
        return false;
    }
    
    // All checks passed
    return true;
}

void SafetyManager::tripRelay() {
    if (!relayTripped_) {
        setRelayState(true);  // Energize relay (disconnect power)
        relayTripped_ = true;
        setRGBStatus(RGB_RED);
        
        #ifdef DEBUG_SERIAL
            Serial.println("[SafetyManager] RELAY TRIPPED - Power disconnected");
        #endif
    }
}

void SafetyManager::resetRelay() {
    setRelayState(false);  // De-energize relay (connect power)
    relayTripped_ = false;
    
    #ifdef DEBUG_SERIAL
        Serial.println("[SafetyManager] Relay reset - Power connected");
    #endif
}

bool SafetyManager::isTripped() const {
    return relayTripped_;
}

void SafetyManager::setRGBStatus(uint8_t r, uint8_t g, uint8_t b) {
    #if RGB_COMMON_CATHODE
        // Common cathode: HIGH = ON
        analogWrite(PIN_RGB_RED, r);
        analogWrite(PIN_RGB_GREEN, g);
        analogWrite(PIN_RGB_BLUE, b);
    #else
        // Common anode: LOW = ON (invert values)
        analogWrite(PIN_RGB_RED, 255 - r);
        analogWrite(PIN_RGB_GREEN, 255 - g);
        analogWrite(PIN_RGB_BLUE, 255 - b);
    #endif
}

String SafetyManager::getLastFaultReason() const {
    return lastFaultReason_;
}

void SafetyManager::clearFault() {
    lastFaultReason_ = "";
    #ifdef DEBUG_SERIAL
        Serial.println("[SafetyManager] Fault cleared");
    #endif
}

bool SafetyManager::checkOverVoltage(float voltage) {
    // Check with hysteresis
    if (relayTripped_) {
        // If already tripped, require voltage to drop below (threshold - hysteresis)
        return voltage > (VOLTAGE_MAX - VOLTAGE_HYSTERESIS);
    } else {
        // If not tripped, trip if voltage exceeds threshold
        return voltage > VOLTAGE_MAX;
    }
}

bool SafetyManager::checkUnderVoltage(float voltage) {
    // Check with hysteresis
    if (relayTripped_) {
        // If already tripped, require voltage to rise above (threshold + hysteresis)
        return voltage < (VOLTAGE_MIN + VOLTAGE_HYSTERESIS);
    } else {
        // If not tripped, trip if voltage falls below threshold
        return voltage < VOLTAGE_MIN;
    }
}

bool SafetyManager::checkOverCurrent(float current) {
    // Check with hysteresis
    if (relayTripped_) {
        // If already tripped, require current to drop below (threshold - hysteresis)
        return current > (CURRENT_MAX - CURRENT_HYSTERESIS);
    } else {
        // If not tripped, trip if current exceeds threshold
        return current > CURRENT_MAX;
    }
}

void SafetyManager::setRelayState(bool energize) {
    #if RELAY_ACTIVE_HIGH
        // Active HIGH: HIGH = energized (tripped)
        digitalWrite(PIN_RELAY, energize ? HIGH : LOW);
    #else
        // Active LOW: LOW = energized (tripped)
        digitalWrite(PIN_RELAY, energize ? LOW : HIGH);
    #endif
}
