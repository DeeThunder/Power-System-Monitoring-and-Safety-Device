#ifndef PERFORMANCE_LOGGER_H
#define PERFORMANCE_LOGGER_H

#include <Arduino.h>

/**
 * @brief Performance data logging module (Serial CSV output)
 * 
 * Outputs CSV-formatted data via Serial for capture on PC:
 * 1. Data transmission latency (sensor read → Blynk transmit)
 * 2. Measurement accuracy (voltage/current stability)
 * 3. Trip response time (fault detection → relay trip)
 * 
 * CSV Format:
 * - [LATENCY],timestamp,sensorReadUs,blynkTransmitMs,totalLatencyMs
 * - [ACCURACY],timestamp,voltage,current,power
 * - [TRIP],timestamp,faultDetectUs,relayTripUs,totalResponseMs
 */
class PerformanceLogger {
public:
    PerformanceLogger();
    
    /**
     * @brief Initialize logger
     */
    void begin();
    
    // ========================================================================
    // LATENCY TRACKING
    // ========================================================================
    
    /**
     * @brief Mark start of sensor reading
     */
    void startSensorRead();
    
    /**
     * @brief Mark end of sensor reading
     */
    void endSensorRead();
    
    /**
     * @brief Mark start of Blynk transmission
     */
    void startBlynkTransmit();
    
    /**
     * @brief Mark end of Blynk transmission and output CSV
     */
    void endBlynkTransmit();
    
    /**
     * @brief Output latency data to Serial in CSV format
     */
    void logLatency();
    
    // ========================================================================
    // ACCURACY TRACKING
    // ========================================================================
    
    /**
     * @brief Log accuracy data (voltage, current, power)
     * @param voltage Voltage reading in Volts
     * @param current Current reading in Amperes
     * @param power Power reading in Watts
     */
    void logAccuracy(float voltage, float current, float power);
    
    // ========================================================================
    // TRIP RESPONSE TRACKING
    // ========================================================================
    
    /**
     * @brief Mark start of fault detection
     */
    void startFaultDetection();
    
    /**
     * @brief Mark end of fault detection
     */
    void endFaultDetection();
    
    /**
     * @brief Mark start of relay trip
     */
    void startRelayTrip();
    
    /**
     * @brief Mark end of relay trip and output CSV
     */
    void endRelayTrip();
    
    /**
     * @brief Output trip response data to Serial in CSV format
     */
    void logTripResponse();
    
private:
    // Timing variables for latency tracking
    unsigned long sensorReadStart_;
    unsigned long sensorReadDuration_;
    unsigned long blynkTransmitStart_;
    unsigned long blynkTransmitDuration_;
    
    // Timing variables for trip response tracking
    unsigned long faultDetectStart_;
    unsigned long faultDetectDuration_;
    unsigned long relayTripStart_;
    unsigned long relayTripDuration_;
    
    /**
     * @brief Output CSV line to Serial
     * @param type Data type marker (LATENCY, ACCURACY, TRIP)
     * @param data CSV data string
     */
    void printCSVLine(const char* type, const String& data);
};

#endif // PERFORMANCE_LOGGER_H
