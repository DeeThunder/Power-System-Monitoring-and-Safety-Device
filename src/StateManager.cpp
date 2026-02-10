#include "StateManager.h"
#include "NetworkManager.h"
#include "config.h"

#ifdef ENABLE_PERFORMANCE_LOGGING
    #include "PerformanceLogger.h"
#endif

#ifdef ENABLE_CLOUD_LOGGING
    #include "CloudLogger.h"
    extern CloudLogger cloudLogger;
#endif

StateManager::StateManager(EnergySensor& sensor, DisplayManager& display, 
                           NetworkManager& network, SafetyManager& safety)
    : sensor_(sensor), display_(display), network_(network), safety_(safety),
      currentState_(STATE_BOOT), previousState_(STATE_BOOT),
      stateEntryTime_(0), lastSensorRead_(0), lastDisplayUpdate_(0),
      lastSafetyCheck_(0),
      blynkWasConnected_(false), powerOutageNotified_(false),
      faultNotified_(false), powerWasPresent_(true),
      lastNotificationTime_(0), isManuallyOff_(false),
      isOverrideActive_(false), lastOverrideWarning_(0) {
}

// Minimum time between notifications to avoid Blynk rate limits
const unsigned long NOTIFICATION_INTERVAL_MS = 1000;

void StateManager::loadSavedStates() {
    isOverrideActive_ = preferences_.getBool(PREF_OVERRIDE_ACTIVE, false);
    isManuallyOff_ = preferences_.getBool(PREF_MANUAL_OFF, false);
    
    #ifdef APP_DEBUG
        Serial.printf("[StateManager] Loaded states: Override=%d, ManualOff=%d\n", 
                      isOverrideActive_, isManuallyOff_);
    #endif
}

void StateManager::saveOverrideState(bool active) {
    preferences_.putBool(PREF_OVERRIDE_ACTIVE, active);
    
    #ifdef APP_DEBUG
        Serial.printf("[StateManager] Saved override state: %d\n", active);
    #endif
}

void StateManager::saveManualSwitchState(bool manualOff) {
    preferences_.putBool(PREF_MANUAL_OFF, manualOff);
    
    #ifdef APP_DEBUG
        Serial.printf("[StateManager] Saved manual switch state: %d\n", manualOff);
    #endif
}

void StateManager::begin() {
    // Initialize preferences
    preferences_.begin(PREF_NAMESPACE, false);  // false = read/write mode
    
    #ifdef APP_DEBUG
        Serial.println("[StateManager] Preferences initialized");
    #endif
    
    // Load saved states from non-volatile storage
    loadSavedStates();
    
    // Set initial state
    setState(STATE_BOOT);
    
    // Restore override state if it was active before power loss
    if (isOverrideActive_) {
        #ifdef APP_DEBUG
            Serial.println("[StateManager] Restoring override state from memory");
        #endif
        
        // Re-enable override (without notification during boot)
        safety_.clearFault();
        safety_.resetRelay();
        safety_.setRGBStatus(RGB_YELLOW);
        // Note: V5 sync will happen after network connects
    }
    
    // Restore manual OFF state if it was set before power loss
    if (isManuallyOff_) {
        #ifdef APP_DEBUG
            Serial.println("[StateManager] Restoring manual OFF state from memory");
        #endif
        
        safety_.tripRelay();
        safety_.setRGBStatus(RGB_ORANGE);
        safety_.setBlinking(true);
        // Note: V5 sync will happen after network connects
    }
    
    // Set up power state change callback for notifications
    safety_.powerStateCallback = [this](bool powerPresent) {
        unsigned long now = millis();
        // Simple throttle check
        bool canSend = (now - lastNotificationTime_ >= NOTIFICATION_INTERVAL_MS);
        
        if (powerPresent) {
            // Power restored
            powerOutageNotified_ = false;  // Reset for next outage
            if (network_.isBlynkConnected() && canSend) {
                network_.sendAlert("POWER RESTORED: Mains voltage detected");
                lastNotificationTime_ = now;
            }
            
            #ifdef ENABLE_CLOUD_LOGGING
                cloudLogger.logSystemEvent("POWER_RESTORED", "Mains voltage detected - Running on AC power");
            #endif
            
            // Note: If throttled, we might miss "POWER RESTORED", but 3s debounce makes collision rare.
            // "POWER OUTAGE" is the critical one to ensure we catch up.
        } else {
            // Power outage
            if (network_.isBlynkConnected() && canSend) {
                network_.sendAlert("POWER OUTAGE: Mains voltage lost");
                powerOutageNotified_ = true;
                lastNotificationTime_ = now;
            } else {
                // Not connected OR throttled - mark as not notified so update() can catch up
                powerOutageNotified_ = false;
            }
            
            #ifdef ENABLE_CLOUD_LOGGING
                cloudLogger.logSystemEvent("POWER_OUTAGE", "Mains voltage lost - Running on battery");
            #endif
        }
        powerWasPresent_ = powerPresent;
    };
    
    #ifdef APP_DEBUG
        Serial.println("[StateManager] Initialized");
    #endif
}

void StateManager::update() {
    // Detect Blynk reconnection edge
    bool blynkConnected = network_.isBlynkConnected();
    bool reconnectionEdge = blynkConnected && !blynkWasConnected_;
    blynkWasConnected_ = blynkConnected;
    
    // Sync-on-Reconnect Logic
    if (reconnectionEdge) {
        #ifdef APP_DEBUG
            Serial.println("[StateManager] Blynk connection detected - verifying status for catch-up alerts");
        #endif
        
        unsigned long now = millis();
        // Check if we can send notification now
        if (now - lastNotificationTime_ >= NOTIFICATION_INTERVAL_MS) {
            
            // 1. Check Power Status
            if (!safety_.isPowerPresent() && !powerOutageNotified_) {
                network_.sendAlert("POWER OUTAGE: Mains voltage lost");
                powerOutageNotified_ = true;
                lastNotificationTime_ = now;
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] Catch-up: Sent POWER OUTAGE alert");
                #endif
                return; // Only send one alert per loop to respect rate limit
            }
            
            // 2. Check Fault Status
            if (currentState_ == STATE_TRIP_PROTECTION && !faultNotified_) {
                network_.sendAlert(safety_.getLastFaultReason());
                faultNotified_ = true;
                lastNotificationTime_ = now;
                #ifdef APP_DEBUG
                    Serial.printf("[StateManager] Catch-up: Sent fault alert: %s\n", 
                                   safety_.getLastFaultReason().c_str());
                #endif
            }
        }
    }
    
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

void StateManager::handleManualSwitch(bool turnOn) {
    #ifdef APP_DEBUG
        Serial.printf("[StateManager] Manual switch: %s\n", turnOn ? "ON" : "OFF");
    #endif
    
    if (turnOn) {
        // User wants to turn system ON
        
        // Safety check: Only allow if conditions are safe
        float voltage = sensor_.getVoltage();
        float current = sensor_.getCurrent();
        
        if (!safety_.checkSafety(voltage, current)) {
            #ifdef APP_DEBUG
                Serial.println("[StateManager] Cannot turn ON - unsafe conditions");
                Serial.printf("[StateManager] V=%.2fV, I=%.2fA\n", voltage, current);
            #endif
            // Sync switch back to OFF
            network_.updateSwitchState(false);
            return;
        }
        
        // Clear manual OFF flag
        isManuallyOff_ = false;
        saveManualSwitchState(false);  // Save to EEPROM
        
        // Clear any previous faults
        safety_.clearFault();
        safety_.resetRelay();  // Turn relay ON
        
        // IMPORTANT: Disable blinking FIRST
        safety_.setBlinking(false);
        
        // Then set the correct LED color based on current state
        if (currentState_ == STATE_TRIP_PROTECTION) {
            if (network_.isWiFiConnected()) {
                setState(STATE_NORMAL);  // Will set Green LED
            } else {
                setState(STATE_OFFLINE_MODE);  // Will set Blue LED
            }
        } else if (currentState_ == STATE_NORMAL) {
            // Already in NORMAL, set green LED
            safety_.setRGBStatus(RGB_GREEN);
        } else if (currentState_ == STATE_OFFLINE_MODE) {
            // Already in OFFLINE, set blue LED
            safety_.setRGBStatus(RGB_BLUE);
        }
        
        #ifdef APP_DEBUG
            Serial.println("[StateManager] Manual ON successful - LED restored to system state");
        #endif
        
    } else {
        // User wants to turn system OFF manually
        isManuallyOff_ = true;  // Set manual OFF flag
        saveManualSwitchState(true);  // Save to EEPROM
        
        safety_.tripRelay();  // Turn relay OFF
        
        // Enable blinking orange LED to indicate manual OFF
        safety_.setRGBStatus(RGB_ORANGE);
        safety_.setBlinking(true);
        
        // Send notification
        network_.sendAlert("LOAD SWITCHED OFF: Manual control via Blynk");
        
        #ifdef APP_DEBUG
            Serial.println("[StateManager] Manual OFF - relay de-energized, blinking orange");
        #endif
    }
}

void StateManager::handleMasterOverride(bool enable) {
    #ifdef APP_DEBUG
        Serial.printf("[StateManager] Master Override: %s\n", enable ? "ENABLED" : "DISABLED");
    #endif
    
    if (enable) {
        // ENABLE OVERRIDE
        isOverrideActive_ = true;
        saveOverrideState(true);  // Save to EEPROM
        lastOverrideWarning_ = millis();
        
        // Clear manual OFF flag (override takes precedence)
        isManuallyOff_ = false;
        
        // Automatically turn ON the load
        safety_.clearFault();
        safety_.resetRelay();  // Energize relay
        safety_.setRGBStatus(RGB_YELLOW);  // Yellow = Override active

        // NOTE: Switch state will be updated by onStateEnter() during state transition
        
        // Transition to appropriate state (exit TRIP_PROTECTION if we were tripped)
        if (network_.isWiFiConnected()) {
            setState(STATE_NORMAL);
        } else {
            setState(STATE_OFFLINE_MODE);
        }
        
        // Send immediate hazard warning
        network_.sendAlert("⚠️ MASTER OVERRIDE ENABLED - Safety protection BYPASSED! Load automatically turned ON. System will operate outside safe limits. DISABLE when not needed!");
        
        // Update state display
        network_.updateState("SAFETY BYPASSED");
        
        // Show override status on OLED display
        display_.showOverrideActive();
        
        
        #ifdef APP_DEBUG
            Serial.println("[StateManager] ⚠️ SAFETY PROTECTION BYPASSED - Load ON");
        #endif
        
    } else {
        // DISABLE OVERRIDE
        isOverrideActive_ = false;
        saveOverrideState(false);  // Save to EEPROM
        
        network_.sendAlert("✅ Master Override DISABLED - Safety protection restored");
        
        // Check current safety conditions
        float voltage = sensor_.getVoltage();
        float current = sensor_.getCurrent();
        
        // If conditions are unsafe, trip immediately
        if (!safety_.checkSafety(voltage, current)) {
            safety_.tripRelay();
            
            // Sync manual switch to OFF
            if (network_.isBlynkConnected()) {
                network_.updateSwitchState(false);
            }
            
            setState(STATE_TRIP_PROTECTION);
            
            #ifdef APP_DEBUG
                Serial.println("[StateManager] Override disabled - conditions unsafe, tripping");
            #endif
        } else {
            // Conditions are safe - restore normal LED color based on current state
            if (currentState_ == STATE_NORMAL) {
                safety_.setRGBStatus(RGB_GREEN);
            } else if (currentState_ == STATE_OFFLINE_MODE) {
                safety_.setRGBStatus(RGB_BLUE);
            }
            
            // Update state display to show normal operation
            if (network_.isWiFiConnected()) {
                network_.updateState("NORMAL");
            } else {
                network_.updateState("OFFLINE");
            }
            
            #ifdef APP_DEBUG
                Serial.println("[StateManager] Safety protection restored - conditions safe");
            #endif
        }
    }
}

void StateManager::updateStateBoot() {
    unsigned long now = millis();
    
    // Show startup screen for 5 seconds (increased from 3 for visibility)
    if (now - stateEntryTime_ < 3000) {
        display_.showStartup();
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
    
    // Update blinking (if enabled)
    safety_.updateBlink();
    
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
        
        // Check if override is active
        if (!isOverrideActive_) {
            // Normal safety check
            if (!safety_.checkSafety(voltage, current)) {
                // FAULT DETECTED - TRIP IMMEDIATELY
                safety_.tripRelay();
                
                // Sync manual switch to OFF in Blynk
                if (network_.isBlynkConnected()) {
                    network_.updateSwitchState(false);
                }
                
                setState(STATE_TRIP_PROTECTION);
                return;  // Exit immediately
            }
        } else {
            // Override active - only check for extreme conditions
            // Still trip on severe overcurrent to prevent fire hazard
            if (current > CURRENT_MAX * 1.5) {  // 150% of max current
                network_.sendAlert("CRITICAL: Extreme overcurrent detected! Tripping despite override.");
                safety_.tripRelay();
                isOverrideActive_ = false;  // Auto-disable override
                saveOverrideState(false);  // Save to EEPROM
                
                // Sync manual switch to OFF in Blynk
                if (network_.isBlynkConnected()) {
                    network_.updateSwitchState(false);
                }
                
                setState(STATE_TRIP_PROTECTION);
                return;
            }
        }
    }
    
    // Periodic override warning
    if (isOverrideActive_) {
        if (now - lastOverrideWarning_ >= OVERRIDE_WARNING_INTERVAL) {
            lastOverrideWarning_ = now;
            network_.sendAlert("⚠️ REMINDER: Master Override is ACTIVE - Safety protection bypassed!");
            
            #ifdef APP_DEBUG
                Serial.println("[StateManager] Override warning sent (30 min periodic)");
            #endif
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
                            sensor_.getPower(), isManuallyOff_);
    }
    
    // Performance logging - log accuracy data periodically
    #ifdef ENABLE_PERFORMANCE_LOGGING
        static unsigned long lastAccuracyLog = 0;
        if (now - lastAccuracyLog >= PERF_ACCURACY_INTERVAL_MS) {
            lastAccuracyLog = now;
            extern PerformanceLogger perfLogger;
            perfLogger.logAccuracy(sensor_.getVoltage(), 
                                  sensor_.getCurrent(), 
                                  sensor_.getPower());
        }
    #endif
    
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
        // CRITICAL FIX: Ensure switch state stays synced during trip
        network_.updateSwitchState(false);  // Keep switch OFF while tripped
        
        network_.publishData(sensor_.getVoltage(), sensor_.getCurrent(), 
                            sensor_.getPower(), true);  // Force zero readings
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
        
        // Check if override is active
        if (!isOverrideActive_) {
            // Normal safety check
            if (!safety_.checkSafety(voltage, current)) {
                // FAULT DETECTED - TRIP IMMEDIATELY
                safety_.tripRelay();
                
                // Sync manual switch to OFF in Blynk (when reconnects)
                if (network_.isBlynkConnected()) {
                    network_.updateSwitchState(false);
                }
                
                setState(STATE_TRIP_PROTECTION);
                return;
            }
        } else {
            // Override active - only check for extreme conditions
            // Still trip on severe overcurrent to prevent fire hazard
            if (current > CURRENT_MAX * 1.5) {  // 150% of max current
                // Note: Can't send alert when offline, but still trip
                safety_.tripRelay();
                isOverrideActive_ = false;  // Auto-disable override
                saveOverrideState(false);  // Save to EEPROM
                
                // Sync manual switch to OFF in Blynk (when reconnects)
                if (network_.isBlynkConnected()) {
                    network_.updateSwitchState(false);
                }
                
                setState(STATE_TRIP_PROTECTION);
                return;
            }
        }
    }
    
    // Periodic override warning (only if Blynk connected)
    if (isOverrideActive_ && network_.isBlynkConnected()) {
        if (now - lastOverrideWarning_ >= OVERRIDE_WARNING_INTERVAL) {
            lastOverrideWarning_ = now;
            network_.sendAlert("⚠️ REMINDER: Master Override is ACTIVE - Safety protection bypassed!");
            
            #ifdef APP_DEBUG
                Serial.println("[StateManager] Override warning sent (30 min periodic)");
            #endif
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
            // Check if override is active - maintain yellow LED
            if (isOverrideActive_) {
                safety_.setRGBStatus(RGB_YELLOW);  // Yellow = Override active
                network_.updateState("OVERRIDE ACTIVE - SAFETY BYPASSED");
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] NORMAL (Override): RGB=Yellow, Relay=ON, Switch=ON");
                #endif
            } else {
                safety_.setRGBStatus(RGB_GREEN);  // Green = Normal operation
                network_.updateState("NORMAL");
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] NORMAL: RGB=Green, Relay=ON, Switch=ON");
                #endif
            }
            safety_.resetRelay();  // Turn relay ON (safe to operate)
            network_.updateSwitchState(true);  // Sync switch to ON
            break;
            
        case STATE_TRIP_PROTECTION:
            safety_.setRGBStatus(RGB_RED);  // Red = Trip/Fault
            safety_.tripRelay();  // Ensure relay is OFF
            
            #ifdef ENABLE_CLOUD_LOGGING
                cloudLogger.logSystemEvent("TRIP", safety_.getLastFaultReason());
            #endif
            
            if (network_.isBlynkConnected()) {
                unsigned long now = millis();
                // Check throttle
                if (now - lastNotificationTime_ >= NOTIFICATION_INTERVAL_MS) {
                    network_.sendAlert(safety_.getLastFaultReason());
                    faultNotified_ = true;
                    lastNotificationTime_ = now;
                } else {
                    faultNotified_ = false;  // Mark for catch-up in update() loop
                }
            } else {
                faultNotified_ = false;  // Mark for catch-up on reconnect
            }
            
            network_.updateState("TRIP: " + safety_.getLastFaultReason());
            network_.updateSwitchState(false);  // Auto-toggle switch to OFF
            #ifdef APP_DEBUG
                Serial.println("[StateManager] TRIP: RGB=Red, Relay=OFF, Switch=OFF");
            #endif
            break;
            
        case STATE_OFFLINE_MODE:
            // Check if override is active - maintain yellow LED
            if (isOverrideActive_) {
                safety_.setRGBStatus(RGB_YELLOW);  // Yellow = Override active
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] OFFLINE (Override): RGB=Yellow, Relay=ON");
                #endif
            } else {
                safety_.setRGBStatus(RGB_BLUE);  // Blue = No WiFi (offline)
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] OFFLINE: RGB=Blue, Relay=ON (not tripped)");
                #endif
            }
            // Only energize relay if not tripped
            if (!safety_.isTripped()) {
                safety_.resetRelay();  // Turn relay ON (can operate offline)
            } else {
                #ifdef APP_DEBUG
                    Serial.println("[StateManager] OFFLINE: Relay=OFF (tripped)");
                #endif
            }
            break;
    }
}

void StateManager::onStateExit() {
    // Cleanup when exiting states (if needed)
}
