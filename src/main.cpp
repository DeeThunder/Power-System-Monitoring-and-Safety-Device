/**
 * Smart Energy Monitoring and Safety System
 * 
 * Author: DeeThunder Nexus (Peace_IoT)
 * Website: deethundernexus.org
 * Client: Samuel
 * Date: Dec 1, 2025
 * 
 * Main application file - coordinates all modules through StateManager
 * 
 * Hardware:
 * - ESP32 Dev Kit
 * - ZMPT101b Voltage Sensor (simulated with potentiometer)
 * - SCT-013 Current Sensor (simulated with potentiometer)
 * - 2-Channel Relay Module
 * - SSD1306 OLED Display (I2C)
 * - RGB LED (Common Cathode)
 * - LED(Power indicator)
 * - Physical Boat Switch (Hardware power control, not used in software)
 * - 2 3.7v Li-ion Batteries
 * - 2 18650 Battery Holders
 * - 1 TP4056 Battery Charger Module
 * - 1 Boost Converter Module
 */

#include <Arduino.h>
#include "config.h"
#include "EnergySensor.h"
#include "DisplayManager.h"
#include "SafetyManager.h"
#include "StateManager.h"
#include "NetworkManager.h"

#ifdef ENABLE_PERFORMANCE_LOGGING
    #include "PerformanceLogger.h"
#endif

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

EnergySensor energySensor;
DisplayManager displayManager;
NetworkManager networkManager;
SafetyManager safetyManager;
StateManager stateManager(energySensor, displayManager, networkManager, safetyManager);

#ifdef ENABLE_PERFORMANCE_LOGGING
    PerformanceLogger perfLogger;
#endif

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Initialize serial communication
    #ifdef APP_DEBUG
        Serial.begin(115200);
        delay(1000);  // Only delay in setup for serial stability
        Serial.println("========================================");
        Serial.println("Smart Energy Monitoring System");
        Serial.println("========================================\n");
    #endif
    
    // ============================================================================
    // DIAGNOSTIC MODE - If enabled, run current sensor diagnostics only
    // ============================================================================
    #ifdef ENABLE_CURRENT_DIAGNOSTIC
        setupCurrentDiagnostic();  // This will run in an infinite loop
        // Code below will never execute when diagnostic mode is enabled
    #endif
    
    // Initialize all modules
    #ifdef APP_DEBUG
        Serial.println("[Main] Initializing modules...");
    #endif
    
    energySensor.begin();
    safetyManager.begin();
    
    if (!displayManager.begin()) {
        #ifdef APP_DEBUG
            Serial.println("[Main] WARNING: Display initialization failed!");
        #endif
    }
    
    networkManager.begin();
    
    // Register reset callback for Blynk button
    networkManager.setResetCallback([]() {
        stateManager.handleReset();
    });
    
    // Register manual switch callback for Blynk switch
    networkManager.setManualSwitchCallback([](bool turnOn) {
        stateManager.handleManualSwitch(turnOn);
    });
    
    // Register master override callback for Blynk switch
    networkManager.setMasterOverrideCallback([](bool enable) {
        stateManager.handleMasterOverride(enable);
    });
    
    stateManager.begin();
    
    #ifdef ENABLE_PERFORMANCE_LOGGING
        perfLogger.begin();
        Serial.println("[Main] Performance logging ENABLED");
        Serial.println("[Main] Run: python tools/performance_logger.py COM3 115200");
    #endif
    
    #ifdef APP_DEBUG
        Serial.println("Safety Thresholds:");
        Serial.printf("  Voltage: %.1fV - %.1fV\n", VOLTAGE_MIN, VOLTAGE_MAX);
        Serial.printf("  Current: 0A - %.1fA\n\n", CURRENT_MAX);
    #endif
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // Update state machine (handles all module coordination)
    stateManager.update();
    // Small yield to prevent watchdog timeout
    yield();
}
