#include "SafetyManager.h"
#include "config.h"

#ifdef ENABLE_PERFORMANCE_LOGGING
    #include "PerformanceLogger.h"
#endif

SafetyManager::SafetyManager() 
    : relayTripped_(false), lastFaultReason_(""), 
      lastVoltage_(0.0), lastCurrent_(0.0),
      blinkEnabled_(false), blinkR_(0), blinkG_(0), blinkB_(0),
      lastBlinkTime_(0), blinkState_(false),
      powerWasPresent_(true), lastPowerChangeTime_(0) {
}

void SafetyManager::begin() {
    // Configure relay pin
    pinMode(PIN_RELAY, OUTPUT);
    
    // CRITICAL: Start with relay OFF during boot - will energize when entering NORMAL state
    setRelayState(false);  // De-energize relay (NO pin open, no power flow)
    relayTripped_ = false;  // Not tripped, just starting OFF
    
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
    #ifdef ENABLE_PERFORMANCE_LOGGING
        extern PerformanceLogger perfLogger;
        perfLogger.startFaultDetection();
    #endif
    
    lastVoltage_ = voltage;
    lastCurrent_ = current;
    
    // CRITICAL: Check if power is present first
    // If voltage is very low (< 100V), the mains is OFF - this is normal, not a fault
    // We should NOT trip in this case, just keep the relay in its current state
    bool powerPresent = (voltage >= VOLTAGE_POWER_PRESENT_THRESHOLD);
    
    // Detect power state changes and notify
    // Use debouncing: only trigger notification if state has been stable for 3 seconds
    const unsigned long POWER_CHANGE_DEBOUNCE_MS = 3000;
    
    if (powerPresent != powerWasPresent_) {
        // Power state changed
        if (millis() - lastPowerChangeTime_ > POWER_CHANGE_DEBOUNCE_MS) {
            // State has been different for long enough - this is a real change
            powerWasPresent_ = powerPresent;
            lastPowerChangeTime_ = millis();
            
            // Trigger notification callback
            if (powerStateCallback != nullptr) {
                powerStateCallback(powerPresent);
            }
            
            #ifdef APP_DEBUG
                if (powerPresent) {
                    Serial.println("[SafetyManager] POWER RESTORED - Mains voltage detected");
                } else {
                    Serial.println("[SafetyManager] POWER OUTAGE - Mains voltage lost");
                }
            #endif
        }
    } else {
        // State is same as before - reset debounce timer
        lastPowerChangeTime_ = millis();
    }
    
    if (!powerPresent) {
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
    #ifdef ENABLE_PERFORMANCE_LOGGING
        perfLogger.endFaultDetection();
    #endif
    
    return true;
}

void SafetyManager::tripRelay() {
    if (!relayTripped_) {
        #ifdef ENABLE_PERFORMANCE_LOGGING
            extern PerformanceLogger perfLogger;
            perfLogger.startRelayTrip();
        #endif
        
        setRelayState(false);  // De-energize relay (OFF -> HIGH signal)
        relayTripped_ = true;
        setRGBStatus(RGB_RED);
        blinkEnabled_ = false;  // Stop blinking when tripped
        
        #ifdef ENABLE_PERFORMANCE_LOGGING
            perfLogger.endRelayTrip();  // This also logs the trip response data
        #endif
        
        #ifdef APP_DEBUG
            Serial.println("[SafetyManager] ⚠️  RELAY TRIPPED");
        #endif
    }
}

void SafetyManager::resetRelay() {
    setRelayState(true);  // Energize relay (ON -> LOW signal)
    relayTripped_ = false;
    blinkEnabled_ = false;  // Stop blinking when relay is ON
    
    #ifdef APP_DEBUG
        Serial.println("[SafetyManager] ✓ Relay RESET");
    #endif
}

bool SafetyManager::isTripped() const {
    return relayTripped_;
}

void SafetyManager::setRGBStatus(uint8_t r, uint8_t g, uint8_t b) {
    // Store the color for blinking
    blinkR_ = r;
    blinkG_ = g;
    blinkB_ = b;
    
    // If blinking is enabled, don't set directly - let updateBlink() handle it
    if (blinkEnabled_) {
        return;
    }
    
    // Set color directly if not blinking
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
    // CRITICAL: Only check undervoltage if power is actually present
    if (voltage < VOLTAGE_POWER_PRESENT_THRESHOLD) {
        return false;  // Not an undervoltage fault - power is just off
    }
    
    // Power is present - now check if it's too low
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

bool SafetyManager::isPowerPresent() const {
    return powerWasPresent_;
}

void SafetyManager::setRelayState(bool energize) {
    #if RELAY_ACTIVE_HIGH
        // Active HIGH + NO pin: HIGH = energized (NO contact closed, power flows)
        digitalWrite(PIN_RELAY, energize ? HIGH : LOW);
    #else
        // Active LOW + NO pin: LOW = energized (NO contact closed, power flows)
        digitalWrite(PIN_RELAY, energize ? LOW : HIGH);
    #endif
    
    // Diagnostic: Read back pin state to verify
    #ifdef APP_DEBUG
        int pinState = digitalRead(PIN_RELAY);
        Serial.printf("[SafetyManager] Relay %s - GPIO%d = %s (Config: Active %s)\n", 
                      energize ? "ENERGIZE" : "DE-ENERGIZE",
                      PIN_RELAY,
                      pinState == HIGH ? "HIGH" : "LOW",
                      RELAY_ACTIVE_HIGH ? "HIGH" : "LOW");
        Serial.printf("[SafetyManager] Expected behavior: Bulb should be %s\n",
                      energize ? "ON" : "OFF");
    #endif
}

void SafetyManager::updateBlink() {
    if (!blinkEnabled_) return;
    
    unsigned long now = millis();
    if (now - lastBlinkTime_ >= 500) {  // Blink every 500ms
        lastBlinkTime_ = now;
        blinkState_ = !blinkState_;
        
        if (blinkState_) {
            // ON state - show color
            #if RGB_COMMON_CATHODE
                analogWrite(PIN_RGB_RED, blinkR_);
                analogWrite(PIN_RGB_GREEN, blinkG_);
                analogWrite(PIN_RGB_BLUE, blinkB_);
            #else
                analogWrite(PIN_RGB_RED, 255 - blinkR_);
                analogWrite(PIN_RGB_GREEN, 255 - blinkG_);
                analogWrite(PIN_RGB_BLUE, 255 - blinkB_);
            #endif
        } else {
            // OFF state - turn off LED
            #if RGB_COMMON_CATHODE
                analogWrite(PIN_RGB_RED, 0);
                analogWrite(PIN_RGB_GREEN, 0);
                analogWrite(PIN_RGB_BLUE, 0);
            #else
                analogWrite(PIN_RGB_RED, 255);
                analogWrite(PIN_RGB_GREEN, 255);
                analogWrite(PIN_RGB_BLUE, 255);
            #endif
        }
    }
}

void SafetyManager::setBlinking(bool enable) {
    blinkEnabled_ = enable;
    
    if (!enable) {
        // When disabling blink, set the color directly
        #if RGB_COMMON_CATHODE
            analogWrite(PIN_RGB_RED, blinkR_);
            analogWrite(PIN_RGB_GREEN, blinkG_);
            analogWrite(PIN_RGB_BLUE, blinkB_);
        #else
            analogWrite(PIN_RGB_RED, 255 - blinkR_);
            analogWrite(PIN_RGB_GREEN, 255 - blinkG_);
            analogWrite(PIN_RGB_BLUE, 255 - blinkB_);
        #endif
    }
    
    #ifdef APP_DEBUG
        Serial.printf("[SafetyManager] Blinking %s\n", enable ? "ENABLED" : "DISABLED");
    #endif
}

