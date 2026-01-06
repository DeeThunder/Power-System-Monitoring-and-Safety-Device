// Blynk configuration must be defined BEFORE including Blynk library
#include "config.h"
#include <BlynkSimpleEsp32.h>  // Defines global Blynk object

#include "NetworkManager.h"

#ifdef ENABLE_PERFORMANCE_LOGGING
    #include "PerformanceLogger.h"
#endif

// Callback for reset button (set from main.cpp)
static void (*resetCallback)() = nullptr;

// Callback for manual switch (set from main.cpp)
static void (*manualSwitchCallback)(bool) = nullptr;

// Blynk handler for reset button
BLYNK_WRITE(VPIN_RESET_BUTTON) {
    int value = param.asInt();
    
    #ifdef APP_DEBUG
        Serial.printf("[NetworkManager] Reset button received: %d\n", value);
    #endif

    if (value == 1 && resetCallback != nullptr) {
        resetCallback();
        // Reset button state in Blynk
        Blynk.virtualWrite(VPIN_RESET_BUTTON, 0);
    }
}

// Blynk handler for manual switch
BLYNK_WRITE(VPIN_MANUAL_SWITCH) {
    int value = param.asInt();
    
    #ifdef APP_DEBUG
        Serial.printf("[NetworkManager] Manual switch received: %d\n", value);
    #endif

    if (manualSwitchCallback != nullptr) {
        manualSwitchCallback(value == 1);
    }
}

NetworkManager::NetworkManager() 
    : wifiConnected_(false), blynkConnected_(false), 
      lastWiFiAttempt_(0), lastBlynkUpdate_(0), lastStateString_("BOOT") {
}

void NetworkManager::begin() {
    WiFi.mode(WIFI_STA);
    
    #ifdef APP_DEBUG
        Serial.println("[NetworkManager] Initializing...");
        Serial.printf("[NetworkManager] Connecting to WiFi: %s\n", WIFI_SSID);
    #endif
    
    // Start WiFi connection (non-blocking)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWiFiAttempt_ = millis();
}

void NetworkManager::setResetCallback(void (*callback)()) {
    resetCallback = callback;
}

void NetworkManager::setManualSwitchCallback(void (*callback)(bool)) {
    manualSwitchCallback = callback;
}

void NetworkManager::update() {
    // Check WiFi status
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiConnected_) {
            wifiConnected_ = true;
            #ifdef APP_DEBUG
                Serial.println("[NetworkManager] WiFi connected!");
                Serial.printf("[NetworkManager] IP Address: %s\n", 
                             WiFi.localIP().toString().c_str());
            #endif
            
            // Initialize Blynk after WiFi connects
            Blynk.config(BLYNK_AUTH_TOKEN);
        }
        
        // Run Blynk (non-blocking)
        if (Blynk.connected()) {
            Blynk.run();
            if (!blynkConnected_) {
                blynkConnected_ = true;
                #ifdef APP_DEBUG
                    Serial.println("[NetworkManager] Blynk connected!");
                #endif
                
                // Sync state immediately upon connection
                if (lastStateString_.length() > 0) {
                     Blynk.virtualWrite(VPIN_STATE, lastStateString_);
                     #ifdef APP_DEBUG
                        Serial.printf("[NetworkManager] Synced state to Blynk: %s\n", lastStateString_.c_str());
                     #endif
                }
            }
        } else {
            if (blynkConnected_) {
                blynkConnected_ = false;
                #ifdef APP_DEBUG
                    Serial.println("[NetworkManager] Blynk disconnected");
                #endif
            }
            // Try to connect to Blynk
            Blynk.connect();
        }
    } else {
        if (wifiConnected_) {
            wifiConnected_ = false;
            blynkConnected_ = false;
            #ifdef APP_DEBUG
                Serial.println("[NetworkManager] WiFi disconnected");
            #endif
        }
        
        // Attempt reconnection periodically
        if (millis() - lastWiFiAttempt_ >= INTERVAL_WIFI_RETRY) {
            reconnectWiFi();
        }
    }
}

bool NetworkManager::isWiFiConnected() const {
    return wifiConnected_;
}

bool NetworkManager::isBlynkConnected() const {
    return blynkConnected_;
}

void NetworkManager::publishData(float voltage, float current, float power) {
    if (!blynkConnected_) return;
    
    // Throttle updates to avoid overwhelming Blynk
    unsigned long now = millis();
    if (now - lastBlynkUpdate_ < INTERVAL_BLYNK) return;
    
    lastBlynkUpdate_ = now;
    
    #ifdef ENABLE_PERFORMANCE_LOGGING
        extern PerformanceLogger perfLogger;
        perfLogger.startBlynkTransmit();
    #endif
    
    // Send data to virtual pins
    Blynk.virtualWrite(VPIN_VOLTAGE, voltage);
    Blynk.virtualWrite(VPIN_CURRENT, current);
    Blynk.virtualWrite(VPIN_POWER, power);
    
    #ifdef ENABLE_PERFORMANCE_LOGGING
        perfLogger.endBlynkTransmit();  // This also logs the latency data
    #endif
    
    #ifdef APP_DEBUG
        Serial.printf("[NetworkManager] Published to Blynk: V=%.2f, I=%.2f, P=%.2f\n", 
                      voltage, current, power);
    #endif
}

void NetworkManager::sendAlert(const String& message) {
    if (!blynkConnected_) return;
    
    // Send notification
    Blynk.logEvent("safety_alert", message);
    
    #ifdef APP_DEBUG
        Serial.printf("[NetworkManager] Alert sent: %s\n", message.c_str());
    #endif
}

void NetworkManager::updateState(const String& state) {
    lastStateString_ = state;
    
    if (!blynkConnected_) return;
    
    Blynk.virtualWrite(VPIN_STATE, state);
    
    #ifdef APP_DEBUG
        Serial.printf("[NetworkManager] State updated: %s\n", state.c_str());
    #endif
}

void NetworkManager::updateSwitchState(bool isOn) {
    if (!blynkConnected_) return;
    
    Blynk.virtualWrite(VPIN_MANUAL_SWITCH, isOn ? 1 : 0);
    
    #ifdef APP_DEBUG
        Serial.printf("[NetworkManager] Switch state updated: %s\n", isOn ? "ON" : "OFF");
    #endif
}

void NetworkManager::reconnectWiFi() {
    #ifdef APP_DEBUG
        Serial.println("[NetworkManager] Attempting WiFi reconnection...");
    #endif
    
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWiFiAttempt_ = millis();
}

bool NetworkManager::connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWiFiAttempt_ = millis();
    return true;
}
