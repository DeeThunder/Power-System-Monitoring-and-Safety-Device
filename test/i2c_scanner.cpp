/**
 * OLED Display I2C Address Scanner
 * 
 * This diagnostic tool scans for I2C devices and helps identify
 * the correct address for your OLED display.
 * 
 * Upload this to your ESP32 to find the correct I2C address.
 */

#include <Arduino.h>
#include <Wire.h>

#define PIN_SDA 21
#define PIN_SCL 22

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n=================================");
    Serial.println("I2C Address Scanner");
    Serial.println("=================================\n");
    
    Wire.begin(PIN_SDA, PIN_SCL);
    
    Serial.println("Scanning I2C bus...\n");
    
    byte count = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");
            
            // Common device identification
            if (address == 0x3C || address == 0x3D) {
                Serial.println("  -> Likely SSD1306 OLED Display");
            }
            
            count++;
        }
    }
    
    Serial.println("\n=================================");
    if (count == 0) {
        Serial.println("No I2C devices found!");
        Serial.println("\nTroubleshooting:");
        Serial.println("1. Check wiring (SDA to GPIO21, SCL to GPIO22)");
        Serial.println("2. Verify power connections (VCC and GND)");
        Serial.println("3. Try different I2C pins if needed");
    } else {
        Serial.printf("Found %d device(s)\n", count);
        Serial.println("\nUpdate SCREEN_ADDRESS in config.h");
        Serial.println("with the address shown above.");
    }
    Serial.println("=================================\n");
}

void loop() {
    // Rescan every 5 seconds
    delay(5000);
    Serial.println("\nRescanning...");
    setup();
}
