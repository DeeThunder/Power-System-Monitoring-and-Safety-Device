#include "CloudLogger.h"
#include "config.h"
#include <ArduinoJson.h>

CloudLogger::CloudLogger() : lastHeartbeat_(0) {}

void CloudLogger::begin() {
    #ifdef APP_DEBUG
        Serial.println("[CloudLogger] Initialized");
        Serial.printf("[CloudLogger] Target: %s\n", GOOGLE_SHEETS_URL);
    #endif
    
    // Log boot event
    logSystemEvent("BOOT", "System started");
}

void CloudLogger::update() {
    // Send heartbeat every 5 minutes (if WiFi connected)
    unsigned long now = millis();
    if (now - lastHeartbeat_ >= CLOUD_LOG_HEARTBEAT_INTERVAL) {
        lastHeartbeat_ = now;
        
        // Build heartbeat details with WiFi and system status
        String details = "WiFi: ";
        if (isWiFiConnected()) {
            details += "Connected (RSSI: " + String(WiFi.RSSI()) + " dBm)";
        } else {
            details += "Disconnected";
        }
        
        logSystemEvent("HEARTBEAT", details);
    }
}

void CloudLogger::logLatency(unsigned long uptime, unsigned long sensorRead, 
                             unsigned long blynkTransmit, unsigned long total) {
    if (!isWiFiConnected()) return;
    
    String values = String(uptime) + "," + 
                   String(sensorRead) + "," +
                   String(blynkTransmit) + "," +
                   String(total);
    
    sendToGoogleSheets("Latency", values);
}

void CloudLogger::logAccuracy(unsigned long uptime, float voltage, 
                              float current, float power) {
    if (!isWiFiConnected()) return;
    
    String values = String(uptime) + "," + 
                   String(voltage, 2) + "," +
                   String(current, 2) + "," +
                   String(power, 2);
    
    sendToGoogleSheets("Accuracy", values);
}

void CloudLogger::logTripResponse(unsigned long uptime, unsigned long faultDetect, 
                                  unsigned long relayTrip, unsigned long total) {
    if (!isWiFiConnected()) return;
    
    String values = String(uptime) + "," + 
                   String(faultDetect) + "," +
                   String(relayTrip) + "," +
                   String(total);
    
    sendToGoogleSheets("TripResponse", values);
}

void CloudLogger::logSystemEvent(const String& event, const String& details) {
    if (!isWiFiConnected()) return;
    
    String values = "\"" + event + "\"," + 
                   String(millis()) + ",\"" + 
                   details + "\"";
    
    sendToGoogleSheets("SystemEvents", values);
}

bool CloudLogger::sendToGoogleSheets(const String& sheet, const String& values) {
    HTTPClient http;
    
    // Prepare JSON payload
    StaticJsonDocument<512> doc;
    doc["sheet"] = sheet;
    
    // Parse CSV values into array
    JsonArray valuesArray = doc.createNestedArray("values");
    int startIdx = 0;
    int commaIdx = values.indexOf(',');
    
    while (commaIdx != -1) {
        String value = values.substring(startIdx, commaIdx);
        value.trim();
        
        // Remove quotes if present
        if (value.startsWith("\"") && value.endsWith("\"")) {
            value = value.substring(1, value.length() - 1);
            valuesArray.add(value);
        } else {
            valuesArray.add(value.toFloat());
        }
        
        startIdx = commaIdx + 1;
        commaIdx = values.indexOf(',', startIdx);
    }
    
    // Add last value
    String lastValue = values.substring(startIdx);
    lastValue.trim();
    if (lastValue.startsWith("\"") && lastValue.endsWith("\"")) {
        lastValue = lastValue.substring(1, lastValue.length() - 1);
        valuesArray.add(lastValue);
    } else {
        valuesArray.add(lastValue.toFloat());
    }
    
    String payload;
    serializeJson(doc, payload);
    
    // Send POST request
    http.begin(GOOGLE_SHEETS_URL);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.POST(payload);
    
    #ifdef APP_DEBUG
        if (httpCode > 0) {
            Serial.printf("[CloudLogger] %s: HTTP %d\n", sheet.c_str(), httpCode);
            if (httpCode == 302 || httpCode == 200) {
                Serial.printf("[CloudLogger] ✓ Data logged successfully\n");
            }
        } else {
            Serial.printf("[CloudLogger] ✗ POST failed: %s\n", http.errorToString(httpCode).c_str());
        }
    #endif
    
    http.end();
    return (httpCode == 302 || httpCode == 200);
}

bool CloudLogger::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}
