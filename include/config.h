// ============================================================================
// BLYNK CONFIGURATION (Must be defined BEFORE including Blynk library)
// ============================================================================
#define BLYNK_TEMPLATE_ID "TMPL2cEqaLW7h"
#define BLYNK_TEMPLATE_NAME "Energy Monitoring and Safety Meter"
#define BLYNK_AUTH_TOKEN "JeQK-J5qRKHFBCm94mNhXztmnpja1udh"

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Analog Sensors (ADC1 only - ADC2 conflicts with WiFi)
#define PIN_VOLTAGE_SENSOR    34  // ADC1_CH6 - Voltage sensor (ZMPT101b / Potentiometer)
#define PIN_CURRENT_SENSOR    35  // ADC1_CH7 - Current sensor (SCT-013 / Potentiometer)

// Digital Outputs
#define PIN_RELAY             26  // Relay control (GPIO23 is safe and reliable)
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

// LOW = Relay energizes (ON), HIGH = Relay de-energizes (OFF)
#define RELAY_ACTIVE_HIGH     true

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
#define VPIN_MANUAL_SWITCH    V5     // Manual ON/OFF switch (write)

// ============================================================================
// SAFETY THRESHOLDS
// ============================================================================

// Voltage Thresholds (in Volts)
#define VOLTAGE_MAX           249.0  // Over-voltage trip threshold
#define VOLTAGE_MIN           200.0  // Under-voltage trip threshold
#define VOLTAGE_HYSTERESIS    5.0    // Hysteresis to prevent relay chattering
#define VOLTAGE_POWER_PRESENT_THRESHOLD 100.0  // Below this = power is off (don't trip)

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
#define VOLTAGE_CALIBRATION_FACTOR  556.0  // Adjusted for ~223V reading -> 228V
#define VOLTAGE_ZERO_POINT          2.19   // VCC/2 for ZMPT101B (adjust if needed)
#define VOLTAGE_NOISE_THRESHOLD     75.0   // Ignore readings below this (ghost voltage)

// Current Sensor Calibration (SCT-013-100: 1V output @ 100A max)
// Hardware: 10kΩ + 10kΩ voltage divider (50% division), 100µF capacitor
// Empirically calibrated: at 0.38A actual, factor 14.5 gave 0.44A reading
// Fine-tuned: 14.5 × (0.38 / 0.44) ≈ 13.0
#define CURRENT_CALIBRATION_FACTOR 13.0  // EmonLib calibration factor
#define CURRENT_EMON_SAMPLES  1660       // Number of samples for EmonLib calcIrms
#define CURRENT_NUM_AVERAGES  5          // Number of readings to average for stability
#define CURRENT_NOISE_THRESHOLD 0.01     // Ignore readings below this (10mA - noise floor)

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
#define INTERVAL_WIFI_RETRY   30000  // WiFi reconnection attempt interval (30 seconds)
#define INTERVAL_BLYNK_RETRY  5000   // Minimum Blynk retry interval (5 seconds, with exponential backoff)

// WiFi Connection Timeout
#define WIFI_CONNECT_TIMEOUT  20000  // WiFi connection timeout (20 seconds)
#define BLYNK_CONNECT_TIMEOUT 3000   // Blynk connection timeout (3 seconds)

// ============================================================================
// RGB LED COLORS (0-255 for each channel)
// ============================================================================

// Adjust these based on Common Cathode/Anode configuration
#if RGB_COMMON_CATHODE
    #define RGB_OFF           0, 0, 0
    #define RGB_BLUE          0, 0, 255      // Booting/Initializing
    #define RGB_GREEN         0, 255, 0      // Normal operation
    #define RGB_YELLOW        255, 255, 0    // Offline mode
    #define RGB_ORANGE        255, 165, 0    // Manual OFF
    #define RGB_RED           255, 0, 0      // Trip protection
#else
    #define RGB_OFF           255, 255, 255
    #define RGB_BLUE          255, 255, 0
    #define RGB_GREEN         255, 0, 255
    #define RGB_YELLOW        0, 0, 255
    #define RGB_ORANGE        0, 90, 255     // Manual OFF (inverted)
    #define RGB_RED           0, 255, 255
#endif

#endif // CONFIG_H
