#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>
#include <functional>

/**
 * @brief Safety Manager Module
 * 
 * Monitors sensor readings against safety thresholds and controls relay/RGB LED.
 */
class SafetyManager {
public:
    SafetyManager();
    void begin();
    bool checkSafety(float voltage, float current);
    void tripRelay();
    void resetRelay();
    bool isTripped() const;
    void setRGBStatus(uint8_t r, uint8_t g, uint8_t b);
    String getLastFaultReason() const;
    void clearFault();
    void updateBlink();
    void setBlinking(bool enable);
    bool isPowerPresent() const;

private:
    bool relayTripped_;
    String lastFaultReason_;
    float lastVoltage_;
    float lastCurrent_;
    
    // Blinking state
    bool blinkEnabled_;
    uint8_t blinkR_, blinkG_, blinkB_;
    unsigned long lastBlinkTime_;
    bool blinkState_;
    
    // Power state tracking for notifications
    bool powerWasPresent_;
    unsigned long lastPowerChangeTime_;
    bool checkUnderVoltage(float voltage);
    bool checkOverVoltage(float voltage);
    bool checkOverCurrent(float current);
    void setRelayState(bool energize);
    
public:
    // Callback for power state changes (for notifications)
    std::function<void(bool)> powerStateCallback = nullptr;
};

#endif // SAFETY_MANAGER_H
