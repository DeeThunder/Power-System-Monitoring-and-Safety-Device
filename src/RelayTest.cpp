/**
 * Simple Relay Test - Direct GPIO Control
 * 
 * This test bypasses all the state management and directly controls the relay
 * to help diagnose the issue.
 * 
 * INSTRUCTIONS:
 * 1. Comment out stateManager.update() in main.cpp loop()
 * 2. Add: testRelayDirect();
 * 3. Upload and watch serial + relay behavior
 */

#include <Arduino.h>
#include "config.h"

void testRelayDirect() {
    static unsigned long lastToggle = 0;
    static bool state = false;
    
    if (millis() - lastToggle >= 1000) {
        lastToggle = millis();
        state = !state;
        
        digitalWrite(PIN_RELAY, state ? HIGH : LOW);
        
        Serial.println("================================");
        Serial.printf("GPIO%d = %s\n", PIN_RELAY, state ? "HIGH" : "LOW");
        Serial.println(">>> DID RELAY CLICK? <<<");
        Serial.println("================================\n");
    }
}

// Alternative: Simple manual test
void testRelayManual() {
    Serial.println("Manual Relay Test - Enter commands:");
    Serial.println("  'h' = Set GPIO HIGH");
    Serial.println("  'l' = Set GPIO LOW");
    Serial.println("  'r' = Read GPIO state");
    
    if (Serial.available()) {
        char cmd = Serial.read();
        
        if (cmd == 'h' || cmd == 'H') {
            digitalWrite(PIN_RELAY, HIGH);
            Serial.printf("GPIO%d set to HIGH\n", PIN_RELAY);
            Serial.println("Did relay click? Is bulb ON or OFF?");
        }
        else if (cmd == 'l' || cmd == 'L') {
            digitalWrite(PIN_RELAY, LOW);
            Serial.printf("GPIO%d set to LOW\n", PIN_RELAY);
            Serial.println("Did relay click? Is bulb ON or OFF?");
        }
        else if (cmd == 'r' || cmd == 'R') {
            int state = digitalRead(PIN_RELAY);
            Serial.printf("GPIO%d is currently: %s\n", PIN_RELAY, state == HIGH ? "HIGH" : "LOW");
        }
    }
}
