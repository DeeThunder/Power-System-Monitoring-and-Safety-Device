#ifndef ENERGY_SENSOR_H
#define ENERGY_SENSOR_H

#include <Arduino.h>

/**
 * @brief Energy Sensor Module
 * 
 * Handles reading voltage and current sensors with calibration support.
 * Supports both simulation mode (potentiometer) and production mode (EmonLib).
 */
class EnergySensor {
public:
    EnergySensor();
    
    /**
     * @brief Initialize the sensor module
     */
    void begin();
    
    /**
     * @brief Update sensor readings (non-blocking)
     * Should be called periodically from main loop
     */
    void update();
    
    /**
     * @brief Get calibrated voltage reading
     * @return Voltage in Volts
     */
    float getVoltage() const;
    
    /**
     * @brief Get calibrated current reading
     * @return Current in Amperes
     */
    float getCurrent() const;
    
    /**
     * @brief Get calculated power
     * @return Power in Watts (V × I)
     */
    float getPower() const;
    
    /**
     * @brief Enable or disable simulation mode
     * @param enable True for simulation (potentiometer), false for production
     */
    void setSimulationMode(bool enable);
    
    /**
     * @brief Check if sensor readings are valid
     * @return True if readings are within expected range
     */
    bool isValid() const;

private:
    float voltage_;
    float current_;
    bool simulationMode_;
    
    /**
     * @brief Read raw ADC value with averaging
     * @param pin ADC pin to read
     * @return Averaged ADC value (0-4095)
     */
    uint16_t readADC(uint8_t pin);
    
    /**
     * @brief Apply calibration to raw ADC reading
     * @param rawValue Raw ADC value
     * @param slope Calibration slope
     * @param intercept Calibration intercept
     * @return Calibrated value
     */
    float applyCalibration(uint16_t rawValue, float slope, float intercept);
    
    /**
     * @brief Read voltage sensor (simulation mode)
     * @return Calibrated voltage
     */
    float readVoltageSimulation();
    
    /**
     * @brief Read current sensor (simulation mode)
     * @return Calibrated current
     */
    float readCurrentSimulation();
    
    // Future: Add EmonLib integration methods
    #ifndef SIMULATION_MODE
    // float readVoltageProduction();
    // float readCurrentProduction();
    #endif
};

#endif // ENERGY_SENSOR_H
