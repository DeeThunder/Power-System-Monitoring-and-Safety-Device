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
    
    /**
     * @brief Initialize safety hardware (relay, RGB LED)
     */
    void begin();
    
    /**
     * @brief Check if readings are within safe limits
     * @param voltage Current voltage reading
     * @param current Current current reading
     * @return True if safe, false if fault detected
     */
    bool checkSafety(float voltage, float current);
    
    /**
     * @brief Trip the relay (disconnect power)
     */
    void tripRelay();
    
    /**
     * @brief Reset the relay (reconnect power)
     */
    void resetRelay();
    
    /**
     * @brief Check if relay is currently tripped
     * @return True if tripped
     */
    bool isTripped() const;
    
    /**
     * @brief Set RGB LED color
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     */
    void setRGBStatus(uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * @brief Get description of last fault
     * @return Fault reason string
     */
    String getLastFaultReason() const;
    
    /**
     * @brief Clear fault condition (allows reset)
     */
    void clearFault();

private:
    bool relayTripped_;
    String lastFaultReason_;
    float lastVoltage_;
    float lastCurrent_;
    
    /**
     * @brief Check for over-voltage condition
     * @param voltage Voltage reading
     * @return True if over-voltage detected
     */
    bool checkOverVoltage(float voltage);
    
    /**
     * @brief Check for under-voltage condition
     * @param voltage Voltage reading
     * @return True if under-voltage detected
     */
    bool checkUnderVoltage(float voltage);
    
    /**
     * @brief Check for over-current condition
     * @param current Current reading
     * @return True if over-current detected
     */
    bool checkOverCurrent(float current);
    
    /**
     * @brief Set relay state
     * @param energize True to energize (trip), false to de-energize
     */
    void setRelayState(bool energize);
};

#endif // SAFETY_MANAGER_H
