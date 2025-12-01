#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
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
     * @brief Handle reset button press
     */
    void handleReset();

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
