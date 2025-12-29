#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>

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
    bool checkOverVoltage(float voltage);
    bool checkUnderVoltage(float voltage);
    bool checkOverCurrent(float current);
    void setRelayState(bool energize);
};

#endif // SAFETY_MANAGER_H
