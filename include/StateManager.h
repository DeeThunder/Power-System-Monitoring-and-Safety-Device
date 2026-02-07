#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "EnergySensor.h"
#include "DisplayManager.h"
#include "SafetyManager.h"

// Forward declaration to avoid including NetworkManager.h (which includes Blynk)
class NetworkManager;

/**
 * @brief System States (Finite State Machine)
 */
enum SystemState {
    STATE_BOOT,              // Initialization and WiFi connection
    STATE_NORMAL,            // Normal monitoring operation
    STATE_TRIP_PROTECTION,   // Safety trip active
    STATE_OFFLINE_MODE       // WiFi disconnected, local monitoring only
};

/**
 * @brief State Manager Module
 * 
 * Implements finite state machine to coordinate all system modules.
 */
class StateManager {
public:
    StateManager(EnergySensor& sensor, DisplayManager& display, 
                 NetworkManager& network, SafetyManager& safety);
    
    /**
     * @brief Initialize state machine
     */
    void begin();
    
    /**
     * @brief Update state machine (call in main loop)
     */
    void update();
    
    /**
     * @brief Transition to new state
     * @param newState Target state
     */
    void setState(SystemState newState);
    
    /**
     * @brief Get current state
     * @return Current system state
     */
    SystemState getState() const;
    
    /**
     * @brief Get human-readable state name
     * @return State name string
     */
    String getStateName() const;
    
    /**
     * @brief Handle reset button press from Blynk
     */
    void handleReset();
    
    /**
     * @brief Handle manual switch toggle from Blynk
     * @param turnOn True to turn system ON, false to turn OFF
     */
    void handleManualSwitch(bool turnOn);
    
    /**
     * @brief Handle master override switch toggle from Blynk
     * @param enable True to enable override, false to disable
     */
    void handleMasterOverride(bool enable);

private:
    EnergySensor& sensor_;
    DisplayManager& display_;
    NetworkManager& network_;
    SafetyManager& safety_;
    
    SystemState currentState_;
    SystemState previousState_;
    unsigned long stateEntryTime_;
    
    // Task timing
    unsigned long lastSensorRead_;
    unsigned long lastDisplayUpdate_;
    unsigned long lastSafetyCheck_;
    
    // Notification tracking
    bool blynkWasConnected_;
    bool powerOutageNotified_;
    bool faultNotified_;
    bool powerWasPresent_;
    unsigned long lastNotificationTime_;
    bool isManuallyOff_;  // Track manual OFF state via Blynk switch
    
    // Master override tracking
    bool isOverrideActive_;              // Override switch state
    unsigned long lastOverrideWarning_;  // Last warning timestamp
    
    // State persistence
    Preferences preferences_;
    
    /**
     * @brief Load saved states from non-volatile storage
     */
    void loadSavedStates();
    
    /**
     * @brief Save override state to non-volatile storage
     */
    void saveOverrideState(bool active);
    
    /**
     * @brief Save manual switch state to non-volatile storage
     */
    void saveManualSwitchState(bool manualOff);
    
    /**
     * @brief Execute STATE_BOOT logic
     */
    void updateStateBoot();
    
    /**
     * @brief Execute STATE_NORMAL logic
     */
    void updateStateNormal();
    
    /**
     * @brief Execute STATE_TRIP_PROTECTION logic
     */
    void updateStateTripProtection();
    
    /**
     * @brief Execute STATE_OFFLINE_MODE logic
     */
    void updateStateOfflineMode();
    
    /**
     * @brief Called when entering a new state
     */
    void onStateEnter();
    
    /**
     * @brief Called when exiting a state
     */
    void onStateExit();
};

#endif // STATE_MANAGER_H
