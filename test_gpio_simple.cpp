/**
 * Ultra-Simple GPIO Test
 * 
 * This is the simplest possible test - just toggles GPIO27 every second.
 * If the relay doesn't click, the problem is hardware (wiring/power).
 * If the relay DOES click, the problem is in the state management code.
 */

#include <Arduino.h>

#define TEST_PIN 27  // GPIO27 for relay

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ULTRA SIMPLE GPIO TEST ===");
    Serial.println("GPIO27 will toggle every 1 second");
    Serial.println("Watch for relay clicks!");
    
    pinMode(TEST_PIN, OUTPUT);
    digitalWrite(TEST_PIN, LOW);  // Start LOW
    
    Serial.println("Started with GPIO27 = LOW");
}

void loop() {
    static unsigned long lastToggle = 0;
    static bool state = false;
    
    if (millis() - lastToggle >= 1000) {
        lastToggle = millis();
        state = !state;
        
        digitalWrite(TEST_PIN, state ? HIGH : LOW);
        
        Serial.println("================================");
        Serial.printf("GPIO27 = %s\n", state ? "HIGH" : "LOW");
        Serial.println(">>> DID RELAY CLICK? <<<");
        Serial.println("================================\n");
    }
}
