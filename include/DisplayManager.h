#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**
 * @brief Display Manager Module
 * 
 * Manages OLED display (SSD1306) with different screen modes.
 */
class DisplayManager {
public:
    DisplayManager();
    
    /**
     * @brief Initialize the OLED display
     * @return True if initialization successful
     */
    bool begin();
    
    /**
     * @brief Display startup/boot screen
     */
    void showStartup();
    
    /**
     * @brief Display live sensor data
     * @param voltage Voltage in Volts
     * @param current Current in Amperes
     * @param power Power in Watts
     * @param wifiConnected WiFi connection status
     * @param blynkConnected Blynk connection status
     */
    void showData(float voltage, float current, float power, 
                  bool wifiConnected, bool blynkConnected);
    
    /**
     * @brief Display trip alert screen
     * @param reason Reason for trip (e.g., "OVER VOLTAGE")
     */
    void showTripAlert(const String& reason);
    
    /**
     * @brief Display offline mode indicator
     */
    void showOfflineMode();
    
    /**
     * @brief Clear the display
     */
    void clear();
    
    /**
     * @brief Check if display is initialized
     * @return True if display is ready
     */
    bool isReady() const;

private:
    Adafruit_SSD1306* display_;
    bool initialized_;
    
    /**
     * @brief Draw WiFi icon
     * @param x X position
     * @param y Y position
     * @param connected Connection status
     */
    void drawWiFiIcon(int16_t x, int16_t y, bool connected);
    
    /**
     * @brief Draw Blynk icon
     * @param x X position
     * @param y Y position
     * @param connected Connection status
     */
    void drawBlynkIcon(int16_t x, int16_t y, bool connected);
};

#endif // DISPLAY_MANAGER_H
