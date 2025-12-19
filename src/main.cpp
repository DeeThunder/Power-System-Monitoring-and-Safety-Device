/**
 * Smart Energy Monitoring and Safety System
 * 
 * Main application file - coordinates all modules through StateManager
 * 
 * Hardware:
 * - ESP32 Dev Kit
 * - ZMPT101b Voltage Sensor (simulated with potentiometer)
 * - SCT-013 Current Sensor (simulated with potentiometer)
 * - 1-Channel Relay Module
 * - SSD1306 OLED Display (I2C)
 * - RGB LED (Common Cathode)
 * - Physical Boat Switch (Hardware power control, not used in software)
 * 
 * Author: Smart Energy System
 * Date: 2025
 */

#include <Arduino.h>
#include "config.h"
#include "EnergySensor.h"
#include "DisplayManager.h"
#include "SafetyManager.h"
#include "StateManager.h"
#include "NetworkManager.h"


// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

EnergySensor energySensor;
DisplayManager displayManager;
NetworkManager networkManager;
SafetyManager safetyManager;
StateManager stateManager(energySensor, displayManager, networkManager, safetyManager);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Initialize serial communication
    #ifdef APP_DEBUG
        Serial.begin(115200);
        delay(1000);  // Only delay in setup for serial stability
        Serial.println("\n\n========================================");
        Serial.println("Smart Energy Monitoring System");
        Serial.println("========================================\n");
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
    
    stateManager.begin();
    
    #ifdef APP_DEBUG
        Serial.println("[Main] Initialization complete\n");
        Serial.println("Pin Configuration:");
        Serial.printf("  Voltage Sensor: GPIO %d\n", PIN_VOLTAGE_SENSOR);
        Serial.printf("  Current Sensor: GPIO %d\n", PIN_CURRENT_SENSOR);
        Serial.printf("  Relay: GPIO %d\n", PIN_RELAY);
        Serial.printf("  RGB LED: R=%d, G=%d, B=%d\n", 
                      PIN_RGB_RED, PIN_RGB_GREEN, PIN_RGB_BLUE);
        Serial.printf("  I2C: SDA=%d, SCL=%d\n\n", PIN_SDA, PIN_SCL);
        
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
