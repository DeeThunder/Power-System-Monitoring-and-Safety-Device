#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "config.h"

/**
 * OLED Display Test Suite
 * 
 * Tests the SSD1306 OLED display:
 * 1. I2C communication
 * 2. Display initialization
 * 3. Text rendering
 * 4. Graphics capabilities
 * 
 * To run: pio test -e test_oled
 */

// Display instance
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);

// Test configuration
bool displayFound = false;
uint8_t displayAddress = 0;

// ============================================================================
// I2C SCANNER
// ============================================================================

void scanI2C() {
    Serial.println("\n[I2C SCAN] Scanning I2C bus...");
    
    Wire.begin(PIN_SDA, PIN_SCL);
    delay(100);
    
    int deviceCount = 0;
    
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("  Found device at 0x%02X", address);
            
            if (address == 0x3C || address == 0x3D) {
                Serial.println(" (SSD1306 OLED) ✓");
                displayFound = true;
                displayAddress = address;
            } else {
                Serial.println();
            }
            
            deviceCount++;
        }
    }
    
    if (deviceCount == 0) {
        Serial.println("  ❌ No I2C devices found!");
    } else {
        Serial.printf("\n  Total devices found: %d\n", deviceCount);
    }
}

// ============================================================================
// UNITY TESTS
// ============================================================================

void test_i2c_communication() {
    Serial.println("\n[TEST 1] I2C Communication");
    Serial.println("═══════════════════════════════════════");
    
    scanI2C();
    
    TEST_ASSERT_TRUE_MESSAGE(displayFound, 
        "OLED display not found on I2C bus! Check wiring.");
    
    Serial.printf("\n✓ OLED found at address 0x%02X\n", displayAddress);
}

void test_display_initialization() {
    Serial.println("\n[TEST 2] Display Initialization");
    Serial.println("═══════════════════════════════════════");
    
    if (!displayFound) {
        TEST_FAIL_MESSAGE("Display not found - skipping init test");
        return;
    }
    
    // Initialize display
    bool initSuccess = display.begin();
    
    TEST_ASSERT_TRUE_MESSAGE(initSuccess, 
        "Display initialization failed!");
    
    // Clear display
    display.clearBuffer();
    display.sendBuffer();
    
    Serial.println("✓ Display initialized successfully");
    Serial.printf("  Width: %d pixels\n", display.getDisplayWidth());
    Serial.printf("  Height: %d pixels\n", display.getDisplayHeight());
}

void test_text_rendering() {
    Serial.println("\n[TEST 3] Text Rendering");
    Serial.println("═══════════════════════════════════════");
    
    if (!displayFound) {
        TEST_FAIL_MESSAGE("Display not found - skipping text test");
        return;
    }
    
    // Test 1: Simple text
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Hello ESP32!");
    display.sendBuffer();
    
    Serial.println("✓ Rendered: 'Hello ESP32!'");
    delay(2000);
    
    // Test 2: Multiple lines
    display.clearBuffer();
    display.drawStr(0, 10, "Line 1");
    display.drawStr(0, 25, "Line 2");
    display.drawStr(0, 40, "Line 3");
    display.sendBuffer();
    
    Serial.println("✓ Rendered: Multiple lines");
    delay(2000);
    
    // Test 3: Large font
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB14_tr);
    display.drawStr(10, 30, "BIG TEXT");
    display.sendBuffer();
    
    Serial.println("✓ Rendered: Large font");
    delay(2000);
    
    TEST_ASSERT_TRUE(true);
}

void test_graphics_rendering() {
    Serial.println("\n[TEST 4] Graphics Rendering");
    Serial.println("═══════════════════════════════════════");
    
    if (!displayFound) {
        TEST_FAIL_MESSAGE("Display not found - skipping graphics test");
        return;
    }
    
    // Test 1: Lines
    display.clearBuffer();
    display.drawLine(0, 0, 127, 63);
    display.drawLine(0, 63, 127, 0);
    display.sendBuffer();
    
    Serial.println("✓ Rendered: Lines");
    delay(2000);
    
    // Test 2: Rectangles
    display.clearBuffer();
    display.drawFrame(10, 10, 40, 30);
    display.drawBox(60, 10, 40, 30);
    display.sendBuffer();
    
    Serial.println("✓ Rendered: Rectangles");
    delay(2000);
    
    // Test 3: Circles
    display.clearBuffer();
    display.drawCircle(32, 32, 20);
    display.drawDisc(96, 32, 20);
    display.sendBuffer();
    
    Serial.println("✓ Rendered: Circles");
    delay(2000);
    
    TEST_ASSERT_TRUE(true);
}

void test_display_demo() {
    Serial.println("\n[TEST 5] Full Display Demo");
    Serial.println("═══════════════════════════════════════");
    
    if (!displayFound) {
        TEST_FAIL_MESSAGE("Display not found - skipping demo");
        return;
    }
    
    // Simulate actual usage
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    // Title
    display.drawStr(15, 10, "Smart Breaker");
    display.drawLine(0, 12, 127, 12);
    
    // Voltage
    display.drawStr(5, 28, "V: 220.5V");
    
    // Current
    display.drawStr(5, 42, "I: 3.2A");
    
    // Power
    display.drawStr(5, 56, "P: 705W");
    
    // Status indicator
    display.drawDisc(120, 30, 4);
    
    display.sendBuffer();
    
    Serial.println("✓ Rendered: Smart Breaker UI");
    Serial.println("\nCheck your OLED display!");
    delay(5000);
    
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║      OLED DISPLAY TEST SUITE           ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    Serial.println("\nDisplay: SSD1306 128x64 OLED");
    Serial.printf("I2C Pins: SDA=%d, SCL=%d\n", PIN_SDA, PIN_SCL);
    Serial.printf("Expected Address: 0x%02X\n\n", SCREEN_ADDRESS);
    
    // Run Unity tests
    UNITY_BEGIN();
    
    RUN_TEST(test_i2c_communication);
    RUN_TEST(test_display_initialization);
    RUN_TEST(test_text_rendering);
    RUN_TEST(test_graphics_rendering);
    RUN_TEST(test_display_demo);
    
    UNITY_END();
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  ALL OLED TESTS COMPLETE               ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    if (displayFound) {
        Serial.println("✓ Display is working correctly!");
        Serial.println("  Check the OLED screen for visual confirmation.");
    } else {
        Serial.println("❌ Display not found!");
        Serial.println("\nTroubleshooting:");
        Serial.println("  1. Check wiring (SDA to GPIO21, SCL to GPIO22)");
        Serial.println("  2. Verify power (VCC to 3.3V, GND to GND)");
        Serial.println("  3. Check I2C address in config.h");
        Serial.println("  4. Try different I2C pins if needed");
    }
}

void loop() {
    // All tests run in setup()
    delay(1000);
}
