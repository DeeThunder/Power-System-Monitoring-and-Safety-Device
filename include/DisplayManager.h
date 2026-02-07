#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

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
     * @brief Display override active warning screen
     */
    void showOverrideActive();
    
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
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C* display_;
    bool initialized_;

    /**
     * @brief Draw the top status bar (WiFi, Battery, Time)
     * @param wifiConnected WiFi connection status
     * @param blynkConnected Blynk connection status
     */
    void drawStatusBar(bool wifiConnected, bool blynkConnected);

    /**
     * @brief Draw signal bars for WiFi
     * @param x X position
     * @param y Y position
     * @param rssi WiFi RSSI strength (or simply connected status)
     */
    void drawSignalBars(int16_t x, int16_t y, int16_t rssi);

    /**
     * @brief Draw battery icon
     * @param x X position
     * @param y Y position
     * @param percentage Battery percentage (mocked)
     */
    void drawBatteryIcon(int16_t x, int16_t y, uint8_t percentage);

    /**
     * @brief Draw a measurement measurement card
     * @param label Label (e.g., "VOLTAGE")
     * @param value Value (e.g., 220.5)
     * @param unit Unit (e.g., "V")
     * @param y Y position
     * @param large Whether to use large font
     */
    void drawMeasurement(const char* label, float value, const char* unit, int16_t y, bool large);
};

#endif // DISPLAY_MANAGER_H
