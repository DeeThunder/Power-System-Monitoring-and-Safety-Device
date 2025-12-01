#include "DisplayManager.h"
#include "config.h"

DisplayManager::DisplayManager() 
    : display_(nullptr), initialized_(false) {
}

bool DisplayManager::begin() {
    // Create display object
    display_ = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    
    // Initialize I2C
    Wire.begin(PIN_SDA, PIN_SCL);
    
    // Initialize display
    if (!display_->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        #ifdef DEBUG_SERIAL
            Serial.println("[DisplayManager] SSD1306 allocation failed!");
        #endif
        initialized_ = false;
        return false;
    }
    
    initialized_ = true;
    
    // Clear display
    display_->clearDisplay();
    display_->display();
    
    #ifdef DEBUG_SERIAL
        Serial.println("[DisplayManager] Initialized successfully");
    #endif
    
    return true;
}

void DisplayManager::showStartup() {
    if (!initialized_) return;
    
    display_->clearDisplay();
    display_->setTextColor(SSD1306_WHITE);
    
    // Title
    display_->setTextSize(2);
    display_->setCursor(10, 10);
    display_->println("ENERGY");
    display_->setCursor(10, 30);
    display_->println("MONITOR");
    
    // Version
    display_->setTextSize(1);
    display_->setCursor(30, 55);
    display_->println("v1.0 - Nigeria");
    
    display_->display();
    
    #ifdef DEBUG_SERIAL
        Serial.println("[DisplayManager] Showing startup screen");
    #endif
}

void DisplayManager::showData(float voltage, float current, float power, 
                              bool wifiConnected, bool blynkConnected) {
    if (!initialized_) return;
    
    display_->clearDisplay();
    display_->setTextColor(SSD1306_WHITE);
    
    // Status icons (top right)
    drawWiFiIcon(100, 0, wifiConnected);
    drawBlynkIcon(115, 0, blynkConnected);
    
    // Voltage
    display_->setTextSize(1);
    display_->setCursor(0, 0);
    display_->println("VOLTAGE:");
    display_->setTextSize(2);
    display_->setCursor(0, 10);
    display_->printf("%.1f V", voltage);
    
    // Current
    display_->setTextSize(1);
    display_->setCursor(0, 28);
    display_->println("CURRENT:");
    display_->setTextSize(2);
    display_->setCursor(0, 38);
    display_->printf("%.2f A", current);
    
    // Power
    display_->setTextSize(1);
    display_->setCursor(0, 56);
    display_->printf("PWR: %.0f W", power);
    
    display_->display();
}

void DisplayManager::showTripAlert(const String& reason) {
    if (!initialized_) return;
    
    display_->clearDisplay();
    display_->setTextColor(SSD1306_WHITE);
    
    // Alert header
    display_->setTextSize(2);
    display_->setCursor(15, 5);
    display_->println("ALERT!");
    
    // Draw warning border
    display_->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display_->drawRect(2, 2, SCREEN_WIDTH-4, SCREEN_HEIGHT-4, SSD1306_WHITE);
    
    // Reason
    display_->setTextSize(1);
    display_->setCursor(10, 30);
    display_->println(reason);
    
    // Instructions
    display_->setCursor(5, 50);
    display_->println("Press RESET");
    
    display_->display();
    
    #ifdef DEBUG_SERIAL
        Serial.printf("[DisplayManager] Showing trip alert: %s\n", reason.c_str());
    #endif
}

void DisplayManager::showOfflineMode() {
    if (!initialized_) return;
    
    display_->clearDisplay();
    display_->setTextColor(SSD1306_WHITE);
    
    // Offline indicator
    display_->setTextSize(1);
    display_->setCursor(20, 0);
    display_->println("OFFLINE MODE");
    
    // WiFi disconnected icon
    drawWiFiIcon(100, 0, false);
    
    // Message
    display_->setTextSize(1);
    display_->setCursor(5, 20);
    display_->println("WiFi Disconnected");
    display_->setCursor(5, 35);
    display_->println("Local monitoring");
    display_->setCursor(5, 45);
    display_->println("active...");
    
    display_->display();
    
    #ifdef DEBUG_SERIAL
        Serial.println("[DisplayManager] Showing offline mode");
    #endif
}

void DisplayManager::clear() {
    if (!initialized_) return;
    display_->clearDisplay();
    display_->display();
}

bool DisplayManager::isReady() const {
    return initialized_;
}

void DisplayManager::drawWiFiIcon(int16_t x, int16_t y, bool connected) {
    if (!initialized_) return;
    
    if (connected) {
        // WiFi connected - draw signal bars
        display_->fillRect(x, y+6, 2, 2, SSD1306_WHITE);
        display_->fillRect(x+3, y+4, 2, 4, SSD1306_WHITE);
        display_->fillRect(x+6, y+2, 2, 6, SSD1306_WHITE);
        display_->fillRect(x+9, y, 2, 8, SSD1306_WHITE);
    } else {
        // WiFi disconnected - draw X
        display_->drawLine(x, y, x+8, y+8, SSD1306_WHITE);
        display_->drawLine(x+8, y, x, y+8, SSD1306_WHITE);
    }
}

void DisplayManager::drawBlynkIcon(int16_t x, int16_t y, bool connected) {
    if (!initialized_) return;
    
    if (connected) {
        // Blynk connected - draw cloud
        display_->fillCircle(x+3, y+4, 3, SSD1306_WHITE);
        display_->fillCircle(x+7, y+4, 3, SSD1306_WHITE);
        display_->fillRect(x+1, y+4, 9, 3, SSD1306_WHITE);
    } else {
        // Blynk disconnected - draw empty cloud
        display_->drawCircle(x+3, y+4, 3, SSD1306_WHITE);
        display_->drawCircle(x+7, y+4, 3, SSD1306_WHITE);
    }
}
