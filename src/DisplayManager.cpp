#include "DisplayManager.h"
#include "config.h"
#include <WiFi.h> // Needed for RSSI

DisplayManager::DisplayManager() 
    : display_(nullptr), initialized_(false) {
}

bool DisplayManager::begin() {
    // Create display object - U8G2_SSD1306_128X64_NONAME_F_HW_I2C
    // F = full buffer mode (faster updates)
    // HW_I2C = hardware I2C
    display_ = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);
    
    // Initialize display
    display_->begin();
    
    initialized_ = true;
    
    // Clear display
    display_->clearBuffer();
    
    #ifdef APP_DEBUG
        Serial.println("[DisplayManager] U8g2 initialized successfully");
    #endif
    
    return true;
}

void DisplayManager::showStartup() {
    if (!initialized_) return;

    display_->clearBuffer();
    
    // Draw border for debugging (to confirm display is working)
    display_->drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Line 1: "POWER SAFETY" - Properly centered
    display_->setFont(u8g2_font_6x10_tr);
    const char* text1 = "POWER SAFETY";
    int w1 = display_->getStrWidth(text1);
    display_->drawStr((SCREEN_WIDTH - w1) / 2, 18, text1);  // Centered
    
    // Line 2: "MONITOR" - Properly centered
    display_->setFont(u8g2_font_6x10_tr);
    const char* text2 = "MONITOR";
    int w2 = display_->getStrWidth(text2);
    display_->drawStr((SCREEN_WIDTH - w2) / 2, 32, text2);  // Centered
    
    // Line 3: "Samuel's Project" - Properly centered
    display_->setFont(u8g2_font_6x10_tr);
    const char* text3 = "Samuel's Project";
    int w3 = display_->getStrWidth(text3);
    display_->drawStr((SCREEN_WIDTH - w3) / 2, 46, text3);  // Centered

    // Line 4: "DeeThunder Nexus" - Properly centered (WITHIN BOUNDS)
    display_->setFont(u8g2_font_5x7_tr);
    const char* text4 = "DeeThunder Nexus";
    int w4 = display_->getStrWidth(text4);
    display_->drawStr((SCREEN_WIDTH - w4) / 2, 58, text4);  // Centered, y=58 is SAFE
    
    display_->sendBuffer();
    
    #ifdef APP_DEBUG
        Serial.println("[DisplayManager] Showing startup screen");
        Serial.printf("[DisplayManager] Text widths: %d, %d, %d, %d\n", w1, w2, w3, w4);
        Serial.printf("[DisplayManager] X positions: %d, %d, %d, %d\n", 
                      (SCREEN_WIDTH - w1) / 2, (SCREEN_WIDTH - w2) / 2, 
                      (SCREEN_WIDTH - w3) / 2, (SCREEN_WIDTH - w4) / 2);
    #endif
}

void DisplayManager::showData(float voltage, float current, float power, 
                              bool wifiConnected, bool blynkConnected) {
    if (!initialized_) return;
    
    display_->clearBuffer();
    
    // 1. Status Bar (Top)
    drawStatusBar(wifiConnected, blynkConnected);
    
    // 2. Content Area (Below Status Bar)
    // Layout: Voltage (Big) on top, Current and Power (Smaller) below
    
    // Voltage - The Hero Metric
    display_->setFont(u8g2_font_6x10_tr);
    display_->drawStr(4, 24, "VOLT");
    
    // Voltage Value (Big)
    display_->setFont(u8g2_font_logisoso16_tr);
    char voltStr[10];
    snprintf(voltStr, sizeof(voltStr), "%.1f", voltage);
    display_->drawStr(45, 30, voltStr);
    
    display_->setFont(u8g2_font_6x10_tr);
    display_->drawStr(110, 30, "V");
    
    // Separator Line
    display_->drawHLine(4, 34, SCREEN_WIDTH - 8);
    
    // Current (Left Bottom)
    display_->setFont(u8g2_font_6x10_tr);
    display_->drawStr(4, 45, "CURR");
    
    char currStr[12];
    snprintf(currStr, sizeof(currStr), "%.2f A", current);
    display_->drawStr(4, 58, currStr);
    
    // Vertical Separator
    display_->drawVLine(64, 40, 20);

    // Power (Right Bottom)
    display_->drawStr(70, 45, "PWR");
    
    char pwrStr[12];
    snprintf(pwrStr, sizeof(pwrStr), "%.1f W", power);
    display_->drawStr(70, 58, pwrStr);

    display_->sendBuffer();
}

void DisplayManager::drawStatusBar(bool wifiConnected, bool blynkConnected) {
    // Status bar height = 12px
    // Background line
    display_->drawHLine(0, 11, SCREEN_WIDTH);

    // Left: "ACTIVE" label
    display_->setFont(u8g2_font_6x10_tr);
    display_->drawStr(2, 9, "ACTIVE");

    // Right: Icons
    // Battery (Far Right) approx x=110
    drawBatteryIcon(110, 1, 100); // Mock 100% since we check mains
    
    // WiFi (Left of Battery) approx x=92
    int16_t rssi = wifiConnected ? WiFi.RSSI() : -100;
    drawSignalBars(92, 2, rssi);

    // Blynk Status (dot next to WiFi)
    if (blynkConnected) {
        display_->drawDisc(85, 6, 2);
    }
}

void DisplayManager::drawSignalBars(int16_t x, int16_t y, int16_t rssi) {
    // RSSI range: -50 (Good) to -100 (Bad)
    int bars = 0;
    if (rssi > -100) bars = 1;
    if (rssi > -85)  bars = 2;
    if (rssi > -70)  bars = 3;
    if (rssi > -55)  bars = 4;
    
    if (rssi == -100 || rssi == 0) bars = 0; // Disconnected

    // Draw 4 bars
    for (int i = 0; i < 4; i++) {
        int h = (i + 1) * 2; // Heights: 2, 4, 6, 8
        if (i < bars) {
            display_->drawBox(x + (i * 3), y + (8 - h), 2, h);
        } else {
            // Draw empty placeholder line at bottom
            display_->drawPixel(x + (i * 3), y + 7);
            display_->drawPixel(x + (i * 3) + 1, y + 7);
        }
    }
    
    // If disconnected, draw a small x
    if (bars == 0) {
        display_->drawLine(x, y, x + 10, y + 8);
        display_->drawLine(x + 10, y, x, y + 8);
    }
}

void DisplayManager::drawBatteryIcon(int16_t x, int16_t y, uint8_t percentage) {
    // Battery Body: 14x8
    display_->drawFrame(x, y + 1, 12, 7);
    // Battery Nipple
    display_->drawBox(x + 12, y + 3, 2, 3);
    
    // Fill based on percentage
    if (percentage > 0) {
        uint8_t w = (percentage * 10) / 100; // Max width 10
        if (w > 10) w = 10;
        display_->drawBox(x + 1, y + 2, w, 5);
    }
}


void DisplayManager::showTripAlert(const String& reason) {
    if (!initialized_) return;
    
    display_->clearBuffer();
    
    // Invert screen for attention - fill background
    display_->drawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Set draw color to black (XOR mode for inverted text)
    display_->setDrawColor(0);
    
    // Alert header
    display_->setFont(u8g2_font_logisoso16_tr);
    const char* alertText = "TRIPPED!";
    int w = display_->getStrWidth(alertText);
    display_->drawStr((SCREEN_WIDTH - w) / 2, 25, alertText);
    
    // Reason
    display_->setFont(u8g2_font_6x10_tr);
    int w2 = display_->getStrWidth(reason.c_str());
    display_->drawStr((SCREEN_WIDTH - w2) / 2, 40, reason.c_str());
    
    // Instructions
    const char* instr = "Check System & Reset";
    int w3 = display_->getStrWidth(instr);
    display_->drawStr((SCREEN_WIDTH - w3) / 2, 55, instr);
    
    // Reset draw color to normal
    display_->setDrawColor(1);
    
    display_->sendBuffer();
    
    #ifdef APP_DEBUG
        Serial.printf("[DisplayManager] Showing trip alert: %s\n", reason.c_str());
    #endif
}

void DisplayManager::showOfflineMode() {
    if (!initialized_) return;
    
    display_->clearBuffer();
    drawStatusBar(false, false);
    
    display_->setFont(u8g2_font_6x10_tr);
    display_->drawStr(10, 25, "OFFLINE MODE");
    
    display_->sendBuffer();

    #ifdef APP_DEBUG
        Serial.println("[DisplayManager] Showing offline mode");
    #endif
}

void DisplayManager::showOverrideActive() {
    if (!initialized_) return;
    
    display_->clearBuffer();
    
    // Invert screen for attention - fill background
    display_->drawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Set draw color to black (XOR mode for inverted text)
    display_->setDrawColor(0);
    
    // Alert header
    display_->setFont(u8g2_font_logisoso16_tr);
    const char* alertText = "OVERRIDE!";
    int w = display_->getStrWidth(alertText);
    display_->drawStr((SCREEN_WIDTH - w) / 2, 25, alertText);
    
    // Warning
    display_->setFont(u8g2_font_6x10_tr);
    const char* warning = "SAFETY BYPASSED";
    int w2 = display_->getStrWidth(warning);
    display_->drawStr((SCREEN_WIDTH - w2) / 2, 40, warning);
    
    // Instructions
    const char* instr = "Disable When Done";
    int w3 = display_->getStrWidth(instr);
    display_->drawStr((SCREEN_WIDTH - w3) / 2, 55, instr);
    
    // Reset draw color to normal
    display_->setDrawColor(1);
    
    display_->sendBuffer();
    
    #ifdef APP_DEBUG
        Serial.println("[DisplayManager] Showing override active warning");
    #endif
}

void DisplayManager::clear() {
    if (!initialized_) return;
    display_->clearBuffer();
    display_->sendBuffer();
}

bool DisplayManager::isReady() const {
    return initialized_;
}
