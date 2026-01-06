#include <Arduino.h>
#include <unity.h>
#include "config.h"

/**
 * Relay Test Suite
 * 
 * Tests relay control functionality to ensure proper operation.
 * 
 * To run: pio test -e esp32dev -f test_relay
 */

void test_relay_toggle() {
    Serial.println("\n[TEST] Relay Toggle Test");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Toggling relay every 2 seconds for 10 cycles\n");
    
    pinMode(PIN_RELAY, OUTPUT);
    
    for (int i = 0; i < 10; i++) {
        bool state = (i % 2 == 0);
        
        #if RELAY_ACTIVE_HIGH
            digitalWrite(PIN_RELAY, state ? HIGH : LOW);
        #else
            digitalWrite(PIN_RELAY, state ? LOW : HIGH);
        #endif
        
        Serial.printf("Cycle %d: GPIO%d = %s (Relay should be %s)\n", 
                     i+1, PIN_RELAY, 
                     digitalRead(PIN_RELAY) == HIGH ? "HIGH" : "LOW",
                     state ? "ON" : "OFF");
        Serial.println(">>> DID RELAY CLICK? <<<\n");
        
        delay(2000);
    }
    
    Serial.println("✓ Toggle test complete");
    TEST_ASSERT_TRUE(true);
}

void test_relay_manual() {
    Serial.println("\n[TEST] Manual Relay Control");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Commands:");
    Serial.println("  'h' = Set relay ON (HIGH)");
    Serial.println("  'l' = Set relay OFF (LOW)");
    Serial.println("  'r' = Read current state");
    Serial.println("  'q' = Quit test\n");
    
    pinMode(PIN_RELAY, OUTPUT);
    
    while (true) {
        if (Serial.available()) {
            char cmd = Serial.read();
            
            if (cmd == 'h' || cmd == 'H') {
                #if RELAY_ACTIVE_HIGH
                    digitalWrite(PIN_RELAY, HIGH);
                #else
                    digitalWrite(PIN_RELAY, LOW);
                #endif
                Serial.println("✓ Relay ON - Did it click? Is load powered?");
            }
            else if (cmd == 'l' || cmd == 'L') {
                #if RELAY_ACTIVE_HIGH
                    digitalWrite(PIN_RELAY, LOW);
                #else
                    digitalWrite(PIN_RELAY, HIGH);
                #endif
                Serial.println("✓ Relay OFF - Did it click? Is load off?");
            }
            else if (cmd == 'r' || cmd == 'R') {
                int state = digitalRead(PIN_RELAY);
                Serial.printf("GPIO%d = %s\n", PIN_RELAY, state == HIGH ? "HIGH" : "LOW");
            }
            else if (cmd == 'q' || cmd == 'Q') {
                Serial.println("Exiting manual test");
                break;
            }
        }
        delay(100);
    }
    
    TEST_ASSERT_TRUE(true);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║         RELAY TEST SUITE               ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    UNITY_BEGIN();
    
    RUN_TEST(test_relay_toggle);
    // Uncomment to run manual test:
    // RUN_TEST(test_relay_manual);
    
    UNITY_END();
}

void loop() {
    // Tests run in setup()
}
