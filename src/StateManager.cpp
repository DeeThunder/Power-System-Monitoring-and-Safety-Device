#include "StateManager.h"
#include "NetworkManager.h"
#include "config.h"

StateManager::StateManager(EnergySensor& sensor, DisplayManager& display, 
                           NetworkManager& network, SafetyManager& safety)
    : sensor_(sensor), display_(display), network_(network), safety_(safety),
      currentState_(STATE_BOOT), previousState_(STATE_BOOT),
      stateEntryTime_(0), lastSensorRead_(0), lastDisplayUpdate_(0),
      lastSafetyCheck_(0) {
}

void StateManager::begin() {
    setState(STATE_BOOT);
    
    #ifdef APP_DEBUG
        Serial.println("[StateManager] Initialized");
    #endif
}

void StateManager::update() {
    // Execute current state logic
    switch (currentState_) {
        case STATE_BOOT:
            updateStateBoot();
            break;
        case STATE_NORMAL:
            updateStateNormal();
            break;
        case STATE_TRIP_PROTECTION:
            updateStateTripProtection();
            break;
        case STATE_OFFLINE_MODE:
            updateStateOfflineMode();
            break;
    }
}

void StateManager::setState(SystemState newState) {
    if (newState == currentState_) return;
    
    previousState_ = currentState_;
    currentState_ = newState;
    stateEntryTime_ = millis();
    
    onStateExit();
    onStateEnter();
    
    #ifdef APP_DEBUG
        Serial.printf("[StateManager] State transition: %s -> %s\n", 
                      getStateName().c_str(), getStateName().c_str());
    #endif
}

SystemState StateManager::getState() const {
    return currentState_;
}

String StateManager::getStateName() const {
    switch (currentState_) {
        case STATE_BOOT:            return "BOOT";
        case STATE_NORMAL:          return "NORMAL";
        case STATE_TRIP_PROTECTION: return "TRIP_PROTECTION";
        case STATE_OFFLINE_MODE:    return "OFFLINE_MODE";
        default:                    return "UNKNOWN";
    }
}

void StateManager::handleReset() {
    #ifdef APP_DEBUG
        Serial.println("[StateManager] Reset button pressed");
    #endif
    
    if (currentState_ == STATE_TRIP_PROTECTION) {
        // Check if conditions are safe to reset
        float voltage = sensor_.getVoltage();
        float current = sensor_.getCurrent();
        
        if (safety_.checkSafety(voltage, current)) {
            // Safe to reset
            safety_.clearFault();
            safety_.resetRelay();
            
            // Return to appropriate state
            if (network_.isWiFiConnected()) {
                setState(STATE_NORMAL);
            } else {
                setState(STATE_OFFLINE_MODE);
            }
            
            #ifdef APP_DEBUG
                Serial.println("[StateManager] System reset successful");
            #endif
        } else {
            #ifdef APP_DEBUG
                Serial.println("[StateManager] Cannot reset - conditions still unsafe");
            #endif
        }
    }
}

void StateManager::updateStateBoot() {
    unsigned long now = millis();
    
    // Show startup screen for 3 seconds
    if (now - stateEntryTime_ < 3000) {
        return;
    }
    
    // Check WiFi connection status
    if (network_.isWiFiConnected()) {
        setState(STATE_NORMAL);
    } else if (now - stateEntryTime_ > WIFI_CONNECT_TIMEOUT) {
        // WiFi connection timeout, go to offline mode
        setState(STATE_OFFLINE_MODE);
    }
}

void StateManager::updateStateNormal() {
    unsigned long now = millis();
    
    // Update network (non-blocking)
    network_.update();
    
    // Read sensors periodically
    if (now - lastSensorRead_ >= INTERVAL_SENSOR_READ) {
        lastSensorRead_ = now;
        sensor_.update();
    }
    
    // Safety check (HIGH PRIORITY - frequent)
    if (now - lastSafetyCheck_ >= INTERVAL_SAFETY) {
        lastSafetyCheck_ = now;
        
        float voltage = sensor_.getVoltage();
        float current = sensor_.getCurrent();
        
        if (!safety_.checkSafety(voltage, current)) {
            // FAULT DETECTED - TRIP IMMEDIATELY
            safety_.tripRelay();
            setState(STATE_TRIP_PROTECTION);
            return;  // Exit immediately
        }
    }
    
    // Update display periodically
    if (now - lastDisplayUpdate_ >= INTERVAL_DISPLAY) {
        lastDisplayUpdate_ = now;
        
        display_.showData(sensor_.getVoltage(), sensor_.getCurrent(), 
                         sensor_.getPower(), network_.isWiFiConnected(), 
                         network_.isBlynkConnected());

        // Serial Debug Output
        #ifdef APP_DEBUG
            Serial.printf("[Data] V: %.1fV | I: %.2fA | P: %.1fW | Relay: %s | WiFi: %s\n", 
                          sensor_.getVoltage(), sensor_.getCurrent(), sensor_.getPower(),
                          safety_.isTripped() ? "OFF" : "ON",
                          network_.isWiFiConnected() ? "CONNECTED" : "DISCONNECTED");
        #endif
    }
    
    // Publish to Blynk
    if (network_.isBlynkConnected()) {
        network_.publishData(sensor_.getVoltage(), sensor_.getCurrent(), 
                            sensor_.getPower());
    }
    
    // Check if WiFi disconnected
    if (!network_.isWiFiConnected()) {
        setState(STATE_OFFLINE_MODE);
    }
}

void StateManager::updateStateTripProtection() {
    unsigned long now = millis();
    
    // Update network (still try to send alerts)
    network_.update();
    
    // Continue reading sensors (to check if safe to reset)
    if (now - lastSensorRead_ >= INTERVAL_SENSOR_READ) {
        lastSensorRead_ = now;
        sensor_.update();
    }
    
    // Update display periodically
    if (now - lastDisplayUpdate_ >= INTERVAL_DISPLAY) {
        lastDisplayUpdate_ = now;
        display_.showTripAlert(safety_.getLastFaultReason());

        #ifdef APP_DEBUG
            Serial.printf("[Trip Mode] V: %.1fV | I: %.2fA | Relay: %s | Reason: %s\n", 
                          sensor_.getVoltage(), sensor_.getCurrent(), 
                          safety_.isTripped() ? "OFF" : "ON",
                          safety_.getLastFaultReason().c_str());
        #endif
    }
    
    // Publish to Blynk (so user knows if it's safe to reset)
    if (network_.isBlynkConnected()) {
        network_.publishData(sensor_.getVoltage(), sensor_.getCurrent(), 
                            sensor_.getPower());
    }
    
    // Note: Reset is handled by handleReset() called from main loop button handler
}

void StateManager::updateStateOfflineMode() {
    unsigned long now = millis();
    
    // Update network (try to reconnect)
    network_.update();
    
    // Read sensors periodically
    if (now - lastSensorRead_ >= INTERVAL_SENSOR_READ) {
        lastSensorRead_ = now;
        sensor_.update();
    }
    
    // Safety check (CRITICAL - must continue even offline!)
    if (now - lastSafetyCheck_ >= INTERVAL_SAFETY) {
        lastSafetyCheck_ = now;
        
        float voltage = sensor_.getVoltage();
        float current = sensor_.getCurrent();
        
        if (!safety_.checkSafety(voltage, current)) {
            // FAULT DETECTED - TRIP IMMEDIATELY
            safety_.tripRelay();
            setState(STATE_TRIP_PROTECTION);
            return;
        }
    }
    
    // Update display periodically
    if (now - lastDisplayUpdate_ >= INTERVAL_DISPLAY) {
        lastDisplayUpdate_ = now;
        display_.showOfflineMode();
    }
    
    // Check if WiFi reconnected
    if (network_.isWiFiConnected()) {
        setState(STATE_NORMAL);
    }
}

void StateManager::onStateEnter() {
    #ifdef APP_DEBUG
        Serial.printf("[StateManager] Entering state: %s\n", getStateName().c_str());
    #endif
    
    switch (currentState_) {
        case STATE_BOOT:
            display_.showStartup();
            safety_.setRGBStatus(RGB_BLUE);  // Blue = Booting/No WiFi
            safety_.tripRelay();  // Keep relay OFF during boot (de-energized)
            #ifdef APP_DEBUG
                Serial.println("[StateManager] BOOT: RGB=Blue, Relay=OFF");
            #endif
            break;
            
        case STATE_NORMAL:
            safety_.setRGBStatus(RGB_GREEN);  // Green = Normal operation
            safety_.resetRelay();  // Turn relay ON (safe to operate)
            network_.updateState("NORMAL");
            #ifdef APP_DEBUG
                Serial.println("[StateManager] NORMAL: RGB=Green, Relay=ON");
            #endif
            break;
            
        case STATE_TRIP_PROTECTION:
            safety_.setRGBStatus(RGB_RED);  // Red = Trip/Fault
            safety_.tripRelay();  // Ensure relay is OFF
            network_.sendAlert(safety_.getLastFaultReason());
            network_.updateState("TRIP: " + safety_.getLastFaultReason());
            #ifdef APP_DEBUG
                Serial.println("[StateManager] TRIP: RGB=Red, Relay=OFF");
            #endif
            break;
            
        case STATE_OFFLINE_MODE:
            safety_.setRGBStatus(RGB_BLUE);  // Blue = No WiFi (offline)
            // Only energize relay if not tripped
            if (!safety_.isTripped()) {
                safety_.resetRelay();  // Turn relay ON (can operate offline)
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] OFFLINE: RGB=Blue, Relay=ON (not tripped)");
                #endif
            } else {
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] OFFLINE: RGB=Blue, Relay=OFF (tripped)");
                #endif
            }
            break;
    }
}

void StateManager::onStateExit() {
    // Cleanup when exiting states (if needed)
}
