#include "SafetyManager.h"
#include "config.h"

SafetyManager::SafetyManager() 
    : relayTripped_(false), lastFaultReason_(""), 
      lastVoltage_(0.0), lastCurrent_(0.0) {
}

void SafetyManager::begin() {
    // Configure relay pin
    pinMode(PIN_RELAY, OUTPUT);
    
    // CRITICAL: Start with relay OFF during boot - will energize when entering NORMAL state
    setRelayState(false);  // Relay OFF (NO pin open, no power flow)
    relayTripped_ = false;
    
    // Configure RGB LED pins
    pinMode(PIN_RGB_RED, OUTPUT);
    pinMode(PIN_RGB_GREEN, OUTPUT);
    pinMode(PIN_RGB_BLUE, OUTPUT);
    
    // Set initial RGB color (Blue = Initializing)
    setRGBStatus(RGB_BLUE);
    
    #ifdef APP_DEBUG
        Serial.println("[SafetyManager] Initialized");
        Serial.printf("[SafetyManager] Relay mode: %s\n", 
                      RELAY_ACTIVE_HIGH ? "Active HIGH" : "Active LOW");
        Serial.println("[SafetyManager] Relay started OFF (will energize when NORMAL)");
    #endif
}

bool SafetyManager::checkSafety(float voltage, float current) {
    lastVoltage_ = voltage;
    lastCurrent_ = current;
    
    // CRITICAL: Check if power is present first
    // If voltage is very low (< 100V), the mains is OFF - this is normal, not a fault
    // We should NOT trip in this case, just keep the relay in its current state
    if (voltage < VOLTAGE_POWER_PRESENT_THRESHOLD) {
        #ifdef APP_DEBUG
            // Only log occasionally to avoid spam
            static unsigned long lastPowerOffLog = 0;
            if (millis() - lastPowerOffLog > 5000) {
                Serial.println("[SafetyManager] Power OFF detected - skipping voltage checks");
                lastPowerOffLog = millis();
            }
        #endif
        return true;  // Safe - power is just off
    }
    
    // Power is ON - now check voltage limits
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
        #ifdef APP_DEBUG
            Serial.printf("[SafetyManager] FAULT: Under-voltage detected (%.2fV)\n", voltage);
        #endif
        return false;
    }
    
    // Check over-current
    if (checkOverCurrent(current)) {
        lastFaultReason_ = "OVER CURRENT";
        #ifdef APP_DEBUG
            Serial.printf("[SafetyManager] FAULT: Over-current detected (%.2fA)\n", current);
        #endif
        return false;
    }
    
    // All checks passed
    return true;
}

void SafetyManager::tripRelay() {
    if (!relayTripped_) {
        setRelayState(false);  // De-energize relay (NO pin opens, disconnect power)
        relayTripped_ = true;
        setRGBStatus(RGB_RED);
        
        #ifdef APP_DEBUG
            Serial.println("[SafetyManager] RELAY TRIPPED - Power disconnected");
        #endif
    }
}

void SafetyManager::resetRelay() {
    setRelayState(true);  // Energize relay (NO pin closes, connect power)
    relayTripped_ = false;
    
    #ifdef APP_DEBUG
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
        // Active HIGH + NO pin: HIGH = energized (NO contact closed, power flows)
        digitalWrite(PIN_RELAY, energize ? HIGH : LOW);
    #else
        // Active LOW + NO pin: LOW = energized (NO contact closed, power flows)
        digitalWrite(PIN_RELAY, energize ? LOW : HIGH);
    #endif
}
