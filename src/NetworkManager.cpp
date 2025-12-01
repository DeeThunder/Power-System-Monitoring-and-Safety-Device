#include "NetworkManager.h"
#include "config.h"

NetworkManager::NetworkManager() 
    : wifiConnected_(false), blynkConnected_(false), 
      lastWiFiAttempt_(0), lastBlynkUpdate_(0) {
}

void NetworkManager::begin() {
    WiFi.mode(WIFI_STA);
    
    #ifdef DEBUG_SERIAL
        Serial.println("[NetworkManager] Initializing...");
        Serial.printf("[NetworkManager] Connecting to WiFi: %s\n", WIFI_SSID);
    #endif
    
    // Start WiFi connection (non-blocking)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWiFiAttempt_ = millis();
}

void NetworkManager::update() {
    // Check WiFi status
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiConnected_) {
            wifiConnected_ = true;
            #ifdef DEBUG_SERIAL
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
                #ifdef DEBUG_SERIAL
                    Serial.println("[NetworkManager] Blynk connected!");
                #endif
            }
        } else {
            if (blynkConnected_) {
                blynkConnected_ = false;
                #ifdef DEBUG_SERIAL
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
            #ifdef DEBUG_SERIAL
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
    
    // Send data to virtual pins
    Blynk.virtualWrite(VPIN_VOLTAGE, voltage);
    Blynk.virtualWrite(VPIN_CURRENT, current);
    Blynk.virtualWrite(VPIN_POWER, power);
    
    #ifdef DEBUG_SERIAL
        Serial.printf("[NetworkManager] Published to Blynk: V=%.2f, I=%.2f, P=%.2f\n", 
                      voltage, current, power);
    #endif
}

void NetworkManager::sendAlert(const String& message) {
    if (!blynkConnected_) return;
    
    // Send notification
    Blynk.logEvent("safety_alert", message);
    
    #ifdef DEBUG_SERIAL
        Serial.printf("[NetworkManager] Alert sent: %s\n", message.c_str());
    #endif
}

void NetworkManager::updateState(const String& state) {
    if (!blynkConnected_) return;
    
    Blynk.virtualWrite(VPIN_STATE, state);
    
    #ifdef DEBUG_SERIAL
        Serial.printf("[NetworkManager] State updated: %s\n", state.c_str());
    #endif
}

void NetworkManager::reconnectWiFi() {
    #ifdef DEBUG_SERIAL
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
