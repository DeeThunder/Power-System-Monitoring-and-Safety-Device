#include "PerformanceLogger.h"
#include "config.h"  // CRITICAL: Must include to get ENABLE_CLOUD_LOGGING flag

#ifdef ENABLE_CLOUD_LOGGING
    #include "CloudLogger.h"
    extern CloudLogger cloudLogger;
#endif

PerformanceLogger::PerformanceLogger()
    : sensorReadStart_(0), sensorReadDuration_(0),
      blynkTransmitStart_(0), blynkTransmitDuration_(0),
      faultDetectStart_(0), faultDetectDuration_(0),
      relayTripStart_(0), relayTripDuration_(0) {
}

void PerformanceLogger::begin() {
    // Nothing to initialize - just using Serial which is already initialized
    #ifdef APP_DEBUG
        Serial.println("[PerformanceLogger] Initialized - CSV output via Serial");
        Serial.println("[PerformanceLogger] CSV Format:");
        Serial.println("  [LATENCY],timestamp,sensorReadUs,blynkTransmitMs,totalLatencyMs");
        Serial.println("  [ACCURACY],timestamp,voltage,current,power");
        Serial.println("  [TRIP],timestamp,faultDetectUs,relayTripUs,totalResponseMs");
    #endif
}

// ============================================================================
// LATENCY TRACKING
// ============================================================================

void PerformanceLogger::startSensorRead() {
    sensorReadStart_ = micros();
}

void PerformanceLogger::endSensorRead() {
    sensorReadDuration_ = micros() - sensorReadStart_;
}

void PerformanceLogger::startBlynkTransmit() {
    blynkTransmitStart_ = millis();
}

void PerformanceLogger::endBlynkTransmit() {
    blynkTransmitDuration_ = millis() - blynkTransmitStart_;
    // Automatically log when Blynk transmit completes
    logLatency();
}

void PerformanceLogger::logLatency() {
    unsigned long totalLatencyMs = (sensorReadDuration_ / 1000) + blynkTransmitDuration_;
    
    String data = String(millis()) + "," + 
                  String(sensorReadDuration_) + "," +
                  String(blynkTransmitDuration_) + "," +
                  String(totalLatencyMs);
    
    printCSVLine("LATENCY", data);

     #ifdef ENABLE_CLOUD_LOGGING
        cloudLogger.logLatency(millis(), sensorReadDuration_, 
                              blynkTransmitDuration_, totalLatencyMs);
    #endif
}

// ============================================================================
// ACCURACY TRACKING
// ============================================================================

void PerformanceLogger::logAccuracy(float voltage, float current, float power) {
    String data = String(millis()) + "," + 
                  String(voltage, 2) + "," +
                  String(current, 2) + "," +
                  String(power, 2);
    
    printCSVLine("ACCURACY", data);

    #ifdef ENABLE_CLOUD_LOGGING
        cloudLogger.logAccuracy(millis(), voltage, current, power);
    #endif
}

// ============================================================================
// TRIP RESPONSE TRACKING
// ============================================================================

void PerformanceLogger::startFaultDetection() {
    faultDetectStart_ = micros();
}

void PerformanceLogger::endFaultDetection() {
    faultDetectDuration_ = micros() - faultDetectStart_;
}

void PerformanceLogger::startRelayTrip() {
    relayTripStart_ = micros();
}

void PerformanceLogger::endRelayTrip() {
    relayTripDuration_ = micros() - relayTripStart_;
    // Automatically log when relay trip completes
    logTripResponse();
}

void PerformanceLogger::logTripResponse() {
    unsigned long totalResponseMs = (faultDetectDuration_ + relayTripDuration_) / 1000;
    
    String data = String(millis()) + "," + 
                  String(faultDetectDuration_) + "," +
                  String(relayTripDuration_) + "," +
                  String(totalResponseMs);
    
    printCSVLine("TRIP", data);

    #ifdef ENABLE_CLOUD_LOGGING
        cloudLogger.logTripResponse(millis(), faultDetectDuration_, 
                                    relayTripDuration_, totalResponseMs);
    #endif
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void PerformanceLogger::printCSVLine(const char* type, const String& data) {
    // Output format: [TYPE],data1,data2,data3,...
    Serial.print("[");
    Serial.print(type);
    Serial.print("],");
    Serial.println(data);
}
