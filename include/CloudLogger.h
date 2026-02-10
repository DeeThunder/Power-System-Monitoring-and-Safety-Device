#ifndef CLOUD_LOGGER_H
#define CLOUD_LOGGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

/**
 * @brief Cloud logger for Google Sheets integration
 * Only logs when WiFi is connected (no offline buffering)
 */
class CloudLogger {
public:
    CloudLogger();
    void begin();
    void update();  // Call in loop() for heartbeat
    
    // Performance logging
    void logLatency(unsigned long uptime, unsigned long sensorRead, 
                   unsigned long blynkTransmit, unsigned long total);
    void logAccuracy(unsigned long uptime, float voltage, float current, float power);
    void logTripResponse(unsigned long uptime, unsigned long faultDetect, 
                        unsigned long relayTrip, unsigned long total);
    
    // System event logging
    void logSystemEvent(const String& event, const String& details = "");
    
private:
    unsigned long lastHeartbeat_;
    
    bool sendToGoogleSheets(const String& sheet, const String& values);
    bool isWiFiConnected();
};

#endif // CLOUD_LOGGER_H
