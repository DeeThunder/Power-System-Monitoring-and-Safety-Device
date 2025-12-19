#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

// Forward declaration of Blynk class (actual include is in .cpp file)
class BlynkWifi;
extern BlynkWifi Blynk;

/**
 * @brief Network Manager Module
 * 
 * Handles WiFi connection and Blynk communication (non-blocking).
 */
class NetworkManager {
public:
    NetworkManager();
    
    /**
     * @brief Initialize WiFi and Blynk
     */
    void begin();
    
    /**
     * @brief Set callback for reset button (called from Blynk handler)
     * @param callback Function to call when reset button is pressed
     */
    void setResetCallback(void (*callback)());
    
    /**
     * @brief Update network connections (non-blocking)
     * Must be called regularly from main loop
     */
    void update();
    
    /**
     * @brief Check if WiFi is connected
     * @return True if connected
     */
    bool isWiFiConnected() const;
    
    /**
     * @brief Check if Blynk is connected
     * @return True if connected
     */
    bool isBlynkConnected() const;
    
    /**
     * @brief Publish sensor data to Blynk
     * @param voltage Voltage reading
     * @param current Current reading
     * @param power Power reading
     */
    void publishData(float voltage, float current, float power);
    
    /**
     * @brief Send alert message to Blynk
     * @param message Alert message
     */
    void sendAlert(const String& message);
    
    /**
     * @brief Update system state on Blynk
     * @param state State name
     */
    void updateState(const String& state);
    
    /**
     * @brief Attempt to reconnect WiFi
     */
    void reconnectWiFi();

private:
    bool wifiConnected_;
    bool blynkConnected_;
    unsigned long lastWiFiAttempt_;
    unsigned long lastBlynkUpdate_;
    String lastStateString_; // Cache for last system state
    
    /**
     * @brief Connect to WiFi (non-blocking)
     * @return True if connection initiated
     */
    bool connectWiFi();
};

#endif // NETWORK_MANAGER_H
