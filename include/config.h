// ============================================================================
// BLYNK CONFIGURATION (Must be defined BEFORE including Blynk library)
// ============================================================================
#define BLYNK_TEMPLATE_ID     "TMPL2cEqaLW7h"  // Replace with your Blynk Template ID
#define BLYNK_TEMPLATE_NAME   "Energy Monitoring and Safety Meter"
#define BLYNK_AUTH_TOKEN      "JeQK-J5qRKHFBCm94mNhXztmnpja1udh"  // Replace with your auth token


#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Analog Sensors (ADC1 only - ADC2 conflicts with WiFi)
#define PIN_VOLTAGE_SENSOR    34  // ADC1_CH6 - Voltage sensor (ZMPT101b / Potentiometer)
#define PIN_CURRENT_SENSOR    35  // ADC1_CH7 - Current sensor (SCT-013 / Potentiometer)

// Digital Outputs
#define PIN_RELAY             26  // Relay control
#define PIN_RGB_RED           25  // RGB LED - Red channel
#define PIN_RGB_GREEN         33  // RGB LED - Green channel
#define PIN_RGB_BLUE          32  // RGB LED - Blue channel

// Digital Inputs
// Note: Physical boat switch controls hardware power to ESP32, not used in software

// I2C (OLED Display)
#define PIN_SDA               21  // I2C Data
#define PIN_SCL               22  // I2C Clock

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================

// Relay Configuration
#define RELAY_ACTIVE_HIGH     true   // Set to false if relay is Active LOW

// RGB LED Configuration
#define RGB_COMMON_CATHODE    true   // Set to false if Common Anode

// OLED Display Configuration
#define SCREEN_WIDTH          128
#define SCREEN_HEIGHT         64
#define OLED_RESET            -1     // Reset pin (-1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS        0x3C   // I2C address (0x3C or 0x3D)

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================

// WiFi Credentials

#include "secret.h"
#define WIFI_SSID             SECRET_WIFI_SSID
#define WIFI_PASSWORD         SECRET_WIFI_PASSWORD

// Blynk Virtual Pins
#define VPIN_VOLTAGE          V0     // Voltage reading
#define VPIN_CURRENT          V1     // Current reading
#define VPIN_POWER            V2     // Power reading
#define VPIN_STATE            V3     // System state (string)
#define VPIN_RESET_BUTTON     V4     // Reset button (write)

// ============================================================================
// SAFETY THRESHOLDS
// ============================================================================

// Voltage Thresholds (in Volts)
#define VOLTAGE_MAX           244.0  // Over-voltage trip threshold
#define VOLTAGE_MIN           216.0  // Under-voltage trip threshold
#define VOLTAGE_HYSTERESIS    5.0    // Hysteresis to prevent relay chattering

// Current Thresholds (in Amperes)
#define CURRENT_MAX           30.0   // Over-current trip threshold
#define CURRENT_HYSTERESIS    0.5    // Hysteresis to prevent relay chattering

// ============================================================================
// CALIBRATION CONSTANTS
// ============================================================================

// Voltage Sensor Calibration (ZMPT101B - RMS based)
// The calibration factor converts RMS voltage reading to actual AC mains voltage
// Typical range: 100-200 (depends on ZMPT101B burden resistor and transformer ratio)
// To calibrate: Measure known AC voltage with multimeter, adjust factor in EnergySensor.cpp
#define VOLTAGE_CALIBRATION_FACTOR  150.0  // Default starting point
#define VOLTAGE_ZERO_POINT          2.5    // VCC/2 for ZMPT101B (adjust if needed)

// Current Sensor Calibration (Linear: actualValue = rawADC * slope + intercept)
#define CURRENT_SLOPE         0.0073 // Calibrated for 0-30A range (30/4095 ≈ 0.0073)
#define CURRENT_INTERCEPT     0.0    // Adjust after calibration

// ADC Configuration
#define ADC_RESOLUTION        4095   // 12-bit ADC (0-4095)
#define ADC_SAMPLES           10     // Number of samples to average

// ============================================================================
// TIMING CONSTANTS (in milliseconds)
// ============================================================================

#define INTERVAL_SENSOR_READ  500    // Sensor reading interval
#define INTERVAL_DISPLAY      1000   // Display update interval
#define INTERVAL_BLYNK        2000   // Blynk data transmission interval
#define INTERVAL_SAFETY       100    // Safety check interval (HIGH PRIORITY)
#define INTERVAL_WIFI_RETRY   30000  // WiFi reconnection attempt interval

// WiFi Connection Timeout
#define WIFI_CONNECT_TIMEOUT  20000  // WiFi connection timeout (20 seconds)

// ============================================================================
// RGB LED COLORS (0-255 for each channel)
// ============================================================================

// Adjust these based on Common Cathode/Anode configuration
#if RGB_COMMON_CATHODE
    #define RGB_OFF           0, 0, 0
    #define RGB_BLUE          0, 0, 255      // Booting/Initializing
    #define RGB_GREEN         0, 255, 0      // Normal operation
    #define RGB_YELLOW        255, 255, 0    // Offline mode
    #define RGB_RED           255, 0, 0      // Trip protection
#else
    #define RGB_OFF           255, 255, 255
    #define RGB_BLUE          255, 255, 0
    #define RGB_GREEN         255, 0, 255
    #define RGB_YELLOW        0, 0, 255
    #define RGB_RED           0, 255, 255
#endif

#endif // CONFIG_H
