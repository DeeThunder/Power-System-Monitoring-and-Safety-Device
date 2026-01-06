#include <Arduino.h>

/**
 * Simple GPIO Test - Verify relay control
 * 
 * This will toggle GPIO26 every 2 seconds
 * Watch the relay and verify it clicks ON/OFF
 */

#define PIN_RELAY 26
#define RELAY_ACTIVE_HIGH true

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    pinMode(PIN_RELAY, OUTPUT);
    
    Serial.println("\n=================================");
    Serial.println("GPIO RELAY TEST");
    Serial.println("=================================");
    Serial.printf("Pin: GPIO%d\n", PIN_RELAY);
    Serial.printf("Active: %s\n\n", RELAY_ACTIVE_HIGH ? "HIGH" : "LOW");
    
    Serial.println("Relay will toggle every 2 seconds");
    Serial.println("Watch for clicks and LED changes\n");
}

void loop() {
    static bool state = false;
    static int count = 0;
    
    count++;
    state = !state;
    
    Serial.println("=================================");
    Serial.printf("Toggle #%d\n", count);
    
    if (state) {
        // Turn relay ON
        Serial.println(">>> TURNING RELAY ON <<<");
        #if RELAY_ACTIVE_HIGH
            digitalWrite(PIN_RELAY, HIGH);
            Serial.println("GPIO26 = HIGH");
        #else
            digitalWrite(PIN_RELAY, LOW);
            Serial.println("GPIO26 = LOW");
        #endif
        
        delay(100);
        int readback = digitalRead(PIN_RELAY);
        Serial.printf("Readback: %s\n", readback == HIGH ? "HIGH" : "LOW");
        Serial.println(">>> DID RELAY CLICK ON? <<<");
        Serial.println(">>> IS LED ON? <<<");
    } else {
        // Turn relay OFF
        Serial.println(">>> TURNING RELAY OFF <<<");
        #if RELAY_ACTIVE_HIGH
            digitalWrite(PIN_RELAY, LOW);
            Serial.println("GPIO26 = LOW");
        #else
            digitalWrite(PIN_RELAY, HIGH);
            Serial.println("GPIO26 = HIGH");
        #endif
        
        delay(100);
        int readback = digitalRead(PIN_RELAY);
        Serial.printf("Readback: %s\n", readback == HIGH ? "HIGH" : "LOW");
        Serial.println(">>> DID RELAY CLICK OFF? <<<");
        Serial.println(">>> IS LED OFF? <<<");
    }
    
    Serial.println("=================================\n");
    delay(2000);
}
