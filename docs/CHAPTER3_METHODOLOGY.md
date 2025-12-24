# CHAPTER 3: METHODOLOGY

## 3.1 Introduction

This chapter presents the systematic approach employed in designing and implementing the Smart Energy Monitoring and Safety System. The methodology encompasses system architecture design, hardware integration, software development, and algorithm implementation. A modular approach was adopted to ensure maintainability, scalability, and ease of debugging.

---

## 3.2 System Architecture

### 3.2.1 Overall System Architecture

The system follows a layered architecture pattern with clear separation of concerns:

```mermaid
graph TB
    subgraph "Application Layer"
        A[Main Application]
        B[State Manager]
    end
    
    subgraph "Business Logic Layer"
        C[Safety Manager]
        D[Network Manager]
        E[Display Manager]
        F[Energy Sensor]
    end
    
    subgraph "Hardware Abstraction Layer"
        G[ADC Interface]
        H[I2C Interface]
        I[GPIO Interface]
        J[WiFi Interface]
    end
    
    subgraph "Hardware Layer"
        K[ZMPT101B Sensor]
        L[SCT-013 Sensor]
        M[SSD1306 Display]
        N[Relay Module]
        O[RGB LED]
        P[ESP32]
    end
    
    A --> B
    B --> C
    B --> D
    B --> E
    B --> F
    C --> I
    D --> J
    E --> H
    F --> G
    G --> K
    G --> L
    H --> M
    I --> N
    I --> O
    P -.-> G
    P -.-> H
    P -.-> I
    P -.-> J
```

### 3.2.2 Module Interaction Diagram

```mermaid
graph LR
    subgraph "Core Modules"
        SM[State Manager<br/>Coordinator]
        ES[Energy Sensor<br/>Data Acquisition]
        SAF[Safety Manager<br/>Protection Logic]
        DM[Display Manager<br/>UI Output]
        NM[Network Manager<br/>IoT Connectivity]
    end
    
    SM -->|Read Sensors| ES
    SM -->|Check Safety| SAF
    SM -->|Update Display| DM
    SM -->|Publish Data| NM
    SAF -->|Trip Relay| SAF
    NM -->|Reset Command| SM
    ES -->|Voltage/Current| SM
    SAF -->|Fault Status| SM
```

---

## 3.3 Hardware Configuration

### 3.3.1 Pin Assignment Table

| Component | GPIO Pin | Function | Type |
|-----------|----------|----------|------|
| ZMPT101B Voltage Sensor | GPIO 34 | Analog Input | ADC1_CH6 |
| SCT-013 Current Sensor | GPIO 35 | Analog Input | ADC1_CH7 |
| Relay Module | GPIO 26 | Digital Output | Control |
| RGB LED - Red | GPIO 25 | PWM Output | Status |
| RGB LED - Green | GPIO 33 | PWM Output | Status |
| RGB LED - Blue | GPIO 32 | PWM Output | Status |
| SSD1306 Display - SDA | GPIO 21 | I2C Data | Communication |
| SSD1306 Display - SCL | GPIO 22 | I2C Clock | Communication |

### 3.3.2 Hardware Block Diagram

```mermaid
graph TB
    subgraph "Power Supply"
        PS[5V Power Supply]
    end
    
    subgraph "Microcontroller"
        ESP[ESP32<br/>Development Board]
    end
    
    subgraph "Sensors"
        VS[ZMPT101B<br/>Voltage Sensor]
        CS[SCT-013<br/>Current Sensor]
    end
    
    subgraph "Actuators"
        RL[Relay Module<br/>NO Contact]
        RGB[RGB LED<br/>Status Indicator]
    end
    
    subgraph "Display"
        OLED[SSD1306<br/>128x64 OLED]
    end
    
    subgraph "Network"
        WIFI[WiFi Module<br/>Built-in ESP32]
        CLOUD[Blynk Cloud<br/>IoT Platform]
    end
    
    PS --> ESP
    PS --> VS
    PS --> CS
    VS -->|Analog 0-3.3V| ESP
    CS -->|Analog 0-3.3V| ESP
    ESP -->|Digital Control| RL
    ESP -->|PWM Signals| RGB
    ESP -->|I2C| OLED
    ESP -->|WiFi| WIFI
    WIFI <-->|HTTPS| CLOUD
    RL -->|Switch| LOAD[Protected Load]
```

---

## 3.4 Software Architecture

### 3.4.1 Class Diagram

```mermaid
classDiagram
    class StateManager {
        -EnergySensor sensor
        -DisplayManager display
        -NetworkManager network
        -SafetyManager safety
        -SystemState currentState
        +begin()
        +update()
        +setState(newState)
        +handleReset()
        -updateStateBoot()
        -updateStateNormal()
        -updateStateTripProtection()
        -updateStateOfflineMode()
    }
    
    class EnergySensor {
        -float voltage
        -float current
        -bool simulationMode
        +begin()
        +update()
        +getVoltage() float
        +getCurrent() float
        +getPower() float
        -readVoltageSimulation() float
        -readCurrentSimulation() float
    }
    
    class SafetyManager {
        -bool relayTripped
        -String lastFaultReason
        +begin()
        +checkSafety(voltage, current) bool
        +tripRelay()
        +resetRelay()
        +isTripped() bool
        +setRGBStatus(r, g, b)
        -checkOverVoltage(voltage) bool
        -checkUnderVoltage(voltage) bool
        -checkOverCurrent(current) bool
    }
    
    class DisplayManager {
        -U8G2 display
        -bool initialized
        +begin() bool
        +showStartup()
        +showData(v, i, p, wifi, blynk)
        +showTripAlert(reason)
        +showOfflineMode()
        -drawStatusBar(wifi, blynk)
        -drawSignalBars(x, y, rssi)
        -drawBatteryIcon(x, y, pct)
    }
    
    class NetworkManager {
        -bool wifiConnected
        -bool blynkConnected
        +begin()
        +update()
        +publishData(v, i, p)
        +sendAlert(message)
        +updateState(state)
        +isWiFiConnected() bool
        +isBlynkConnected() bool
    }
    
    StateManager --> EnergySensor
    StateManager --> SafetyManager
    StateManager --> DisplayManager
    StateManager --> NetworkManager
```

### 3.4.2 State Machine Diagram

```mermaid
stateDiagram-v2
    [*] --> BOOT
    
    BOOT --> NORMAL : WiFi Connected
    BOOT --> OFFLINE_MODE : WiFi Timeout
    
    NORMAL --> TRIP_PROTECTION : Safety Fault Detected
    NORMAL --> OFFLINE_MODE : WiFi Disconnected
    
    OFFLINE_MODE --> NORMAL : WiFi Reconnected
    OFFLINE_MODE --> TRIP_PROTECTION : Safety Fault Detected
    
    TRIP_PROTECTION --> NORMAL : Reset + Safe Conditions + WiFi
    TRIP_PROTECTION --> OFFLINE_MODE : Reset + Safe Conditions + No WiFi
    
    note right of BOOT
        - Initialize modules
        - Show startup screen
        - Attempt WiFi connection
        - Relay OFF
    end note
    
    note right of NORMAL
        - Monitor sensors
        - Check safety limits
        - Update display
        - Publish to cloud
        - Relay ON
    end note
    
    note right of OFFLINE_MODE
        - Continue monitoring
        - Safety checks active
        - Local display only
        - Attempt reconnection
        - Relay ON (if not tripped)
    end note
    
    note right of TRIP_PROTECTION
        - Relay OFF (tripped)
        - Display fault reason
        - Send alert to cloud
        - Wait for manual reset
    end note
```

---

## 3.5 Algorithm Design

### 3.5.1 Main Control Loop Algorithm

**Pseudocode:**
```
ALGORITHM MainControlLoop
BEGIN
    INITIALIZE all modules (sensors, display, network, safety)
    SET state = BOOT
    DISPLAY startup screen
    
    WHILE system is running DO
        CALL StateManager.update()
        
        SWITCH current_state DO
            CASE BOOT:
                IF WiFi connected THEN
                    TRANSITION to NORMAL state
                ELSE IF timeout exceeded THEN
                    TRANSITION to OFFLINE_MODE state
                END IF
                
            CASE NORMAL:
                READ sensor values (voltage, current)
                CHECK safety conditions
                IF fault detected THEN
                    TRIP relay
                    TRANSITION to TRIP_PROTECTION state
                ELSE IF WiFi disconnected THEN
                    TRANSITION to OFFLINE_MODE state
                ELSE
                    UPDATE display with sensor data
                    PUBLISH data to cloud
                    ENERGIZE relay
                END IF
                
            CASE OFFLINE_MODE:
                READ sensor values
                CHECK safety conditions
                IF fault detected THEN
                    TRIP relay
                    TRANSITION to TRIP_PROTECTION state
                ELSE IF WiFi reconnected THEN
                    TRANSITION to NORMAL state
                ELSE
                    UPDATE display (offline mode)
                    ENERGIZE relay (if not tripped)
                END IF
                
            CASE TRIP_PROTECTION:
                READ sensor values (for monitoring)
                DISPLAY fault alert
                PUBLISH fault to cloud
                KEEP relay OFF
                IF reset button pressed AND conditions safe THEN
                    CLEAR fault
                    TRANSITION to NORMAL or OFFLINE_MODE
                END IF
        END SWITCH
        
        YIELD to prevent watchdog timeout
    END WHILE
END
```

### 3.5.2 RMS Calculation Algorithm (Single-Pass)

**Mathematical Foundation:**

The Root Mean Square (RMS) value of an AC signal is calculated using the formula:

$$
V_{RMS} = \sqrt{\frac{1}{N}\sum_{i=1}^{N}(v_i - \bar{v})^2}
$$

Where:
- $v_i$ = individual sample value
- $\bar{v}$ = mean (DC bias)
- $N$ = number of samples

This can be optimized to a single-pass algorithm using:

$$
V_{RMS} = \sqrt{\frac{\sum v_i^2}{N} - \left(\frac{\sum v_i}{N}\right)^2}
$$

**Pseudocode:**
```
ALGORITHM CalculateRMS_SinglePass
INPUT: sensor_pin, num_samples, calibration_factor
OUTPUT: calibrated_voltage

BEGIN
    sum_raw ← 0
    sum_squared_raw ← 0
    
    // Single sampling pass
    FOR i = 1 TO num_samples DO
        raw_adc ← READ_ANALOG(sensor_pin)
        sum_raw ← sum_raw + raw_adc
        sum_squared_raw ← sum_squared_raw + (raw_adc × raw_adc)
        DELAY_MICROSECONDS(100)
    END FOR
    
    // Calculate mean (DC bias)
    mean ← sum_raw / num_samples
    
    // Calculate variance
    mean_square ← (sum_squared_raw / num_samples) - (mean × mean)
    
    // Handle numerical errors
    IF mean_square < 0 THEN
        mean_square ← 0
    END IF
    
    // Calculate RMS in ADC counts
    rms_adc ← SQRT(mean_square)
    
    // Convert to voltage
    rms_voltage ← (rms_adc / 4095) × 3.3
    
    // Apply calibration
    actual_voltage ← rms_voltage × calibration_factor
    
    // Noise gate
    IF actual_voltage < NOISE_THRESHOLD THEN
        actual_voltage ← 0
    END IF
    
    RETURN actual_voltage
END
```

### 3.5.3 Safety Check Algorithm with Power Detection

**Flowchart:**

```mermaid
flowchart TD
    Start([Start Safety Check]) --> ReadSensors[Read Voltage & Current]
    ReadSensors --> PowerCheck{Voltage < 100V?}
    
    PowerCheck -->|Yes| PowerOff[Power is OFF]
    PowerOff --> ReturnSafe[Return SAFE]
    
    PowerCheck -->|No| PowerOn[Power is ON]
    PowerOn --> OverVoltage{Voltage > Max?}
    
    OverVoltage -->|Yes| SetFault1[Set Fault: OVER VOLTAGE]
    SetFault1 --> ReturnFault[Return FAULT]
    
    OverVoltage -->|No| UnderVoltage{Voltage < Min?}
    
    UnderVoltage -->|Yes| SetFault2[Set Fault: UNDER VOLTAGE]
    SetFault2 --> ReturnFault
    
    UnderVoltage -->|No| OverCurrent{Current > Max?}
    
    OverCurrent -->|Yes| SetFault3[Set Fault: OVER CURRENT]
    SetFault3 --> ReturnFault
    
    OverCurrent -->|No| AllSafe[All Checks Passed]
    AllSafe --> ReturnSafe
    
    ReturnSafe --> End([End])
    ReturnFault --> End
```

**Pseudocode:**
```
ALGORITHM CheckSafety
INPUT: voltage, current
OUTPUT: is_safe (boolean), fault_reason (string)

BEGIN
    // Power present detection
    IF voltage < POWER_PRESENT_THRESHOLD THEN
        // Power is OFF - normal condition
        RETURN (TRUE, "")
    END IF
    
    // Power is ON - check safety limits
    
    // Check over-voltage with hysteresis
    IF relay_tripped THEN
        over_voltage_limit ← VOLTAGE_MAX - VOLTAGE_HYSTERESIS
    ELSE
        over_voltage_limit ← VOLTAGE_MAX
    END IF
    
    IF voltage > over_voltage_limit THEN
        fault_reason ← "OVER VOLTAGE"
        RETURN (FALSE, fault_reason)
    END IF
    
    // Check under-voltage with hysteresis
    IF relay_tripped THEN
        under_voltage_limit ← VOLTAGE_MIN + VOLTAGE_HYSTERESIS
    ELSE
        under_voltage_limit ← VOLTAGE_MIN
    END IF
    
    IF voltage < under_voltage_limit THEN
        fault_reason ← "UNDER VOLTAGE"
        RETURN (FALSE, fault_reason)
    END IF
    
    // Check over-current with hysteresis
    IF relay_tripped THEN
        over_current_limit ← CURRENT_MAX - CURRENT_HYSTERESIS
    ELSE
        over_current_limit ← CURRENT_MAX
    END IF
    
    IF current > over_current_limit THEN
        fault_reason ← "OVER CURRENT"
        RETURN (FALSE, fault_reason)
    END IF
    
    // All checks passed
    RETURN (TRUE, "")
END
```

### 3.5.4 Exponential Moving Average (EMA) Smoothing

**Mathematical Formula:**

$$
S_t = \alpha \cdot x_t + (1 - \alpha) \cdot S_{t-1}
$$

Where:
- $S_t$ = smoothed value at time $t$
- $x_t$ = raw measurement at time $t$
- $\alpha$ = smoothing factor (0 < α < 1)
- $S_{t-1}$ = previous smoothed value

**Pseudocode:**
```
ALGORITHM ApplyEMASmoothing
INPUT: raw_voltage, raw_current, alpha
OUTPUT: smoothed_voltage, smoothed_current

BEGIN
    // Check for power-off condition
    IF raw_voltage < NOISE_THRESHOLD THEN
        // Immediate reset on power loss
        smoothed_voltage ← 0
        smoothed_current ← 0
    ELSE
        // Normal smoothing
        
        // Initialize on first reading
        IF smoothed_voltage = 0 AND raw_voltage > 0 THEN
            smoothed_voltage ← raw_voltage
        END IF
        
        IF smoothed_current = 0 AND raw_current > 0 THEN
            smoothed_current ← raw_current
        END IF
        
        // Apply EMA filter
        smoothed_voltage ← alpha × raw_voltage + (1 - alpha) × smoothed_voltage
        smoothed_current ← alpha × raw_current + (1 - alpha) × smoothed_current
    END IF
    
    RETURN (smoothed_voltage, smoothed_current)
END
```

---

## 3.6 Data Flow Diagram

### 3.6.1 Level 0 DFD (Context Diagram)

```mermaid
graph LR
    subgraph External Entities
        USER[User]
        MAINS[AC Mains Supply]
        CLOUD[Blynk Cloud]
        LOAD[Protected Load]
    end
    
    subgraph System
        SYS[Energy Monitoring<br/>& Safety System]
    end
    
    MAINS -->|AC Voltage/Current| SYS
    SYS -->|Relay Control| LOAD
    SYS -->|Display Data| USER
    USER -->|Reset Command| SYS
    SYS <-->|Sensor Data<br/>Control Commands| CLOUD
    CLOUD -->|Reset Command| SYS
```

### 3.6.2 Level 1 DFD (System Processes)

```mermaid
graph TB
    subgraph Inputs
        I1[AC Voltage Signal]
        I2[AC Current Signal]
        I3[Reset Button]
        I4[WiFi Network]
    end
    
    subgraph Processes
        P1[1.0<br/>Acquire Sensor Data]
        P2[2.0<br/>Process & Filter Data]
        P3[3.0<br/>Check Safety Limits]
        P4[4.0<br/>Control Relay]
        P5[5.0<br/>Update Display]
        P6[6.0<br/>Manage Network]
    end
    
    subgraph Data Stores
        D1[(Sensor Readings)]
        D2[(System State)]
        D3[(Safety Status)]
    end
    
    subgraph Outputs
        O1[OLED Display]
        O2[RGB LED]
        O3[Relay Output]
        O4[Cloud Dashboard]
    end
    
    I1 --> P1
    I2 --> P1
    P1 --> D1
    D1 --> P2
    P2 --> D1
    D1 --> P3
    P3 --> D3
    D3 --> P4
    P4 --> O3
    D1 --> P5
    D2 --> P5
    D3 --> P5
    P5 --> O1
    P5 --> O2
    I4 --> P6
    D1 --> P6
    D2 --> P6
    D3 --> P6
    P6 --> O4
    I3 --> P4
```

---

## 3.7 Implementation Methodology

### 3.7.1 Development Approach

The system was developed using an **Iterative and Incremental** approach:

1. **Module-Based Development**
   - Each component (sensor, display, safety, network) developed independently
   - Unit testing performed on individual modules
   - Integration performed incrementally

2. **Test-Driven Refinement**
   - Initial implementation with basic functionality
   - Real-world testing to identify issues
   - Iterative refinement based on empirical data

3. **Calibration-First Strategy**
   - Hardware validation before software integration
   - Sensor calibration using reference instruments
   - Threshold tuning based on actual operating conditions

### 3.7.2 Software Development Workflow

```mermaid
graph TD
    A[Requirements Analysis] --> B[Module Design]
    B --> C[Implementation]
    C --> D[Unit Testing]
    D --> E{Tests Pass?}
    E -->|No| C
    E -->|Yes| F[Integration]
    F --> G[System Testing]
    G --> H{Issues Found?}
    H -->|Yes| I[Debug & Analyze]
    I --> J[Refine Algorithm]
    J --> C
    H -->|No| K[Calibration]
    K --> L[Field Testing]
    L --> M{Performance OK?}
    M -->|No| I
    M -->|Yes| N[Deployment]
```

### 3.7.3 Calibration Procedure

**Step 1: Voltage Sensor Calibration**
```
1. Connect ZMPT101B to known AC source (verified with multimeter)
2. Enable raw RMS logging in firmware
3. Record raw RMS value from serial monitor
4. Calculate calibration factor:
   Calibration Factor = Actual Voltage (multimeter) / Raw RMS
5. Update VOLTAGE_CALIBRATION_FACTOR in config.h
6. Verify reading matches multimeter
7. Repeat if necessary
```

**Step 2: Current Sensor Calibration**
```
1. Connect known load (measured with clamp meter)
2. Enable raw RMS logging for current
3. Record raw RMS value
4. Calculate calibration factor:
   Calibration Factor = Actual Current (meter) / Raw RMS
5. Update current calibration in code
6. Verify with multiple load conditions
```

**Step 3: Threshold Tuning**
```
1. Measure noise floor with power OFF
2. Set VOLTAGE_NOISE_THRESHOLD above noise floor
3. Test trip thresholds with variable input
4. Adjust VOLTAGE_MIN/MAX based on grid stability
5. Configure hysteresis to prevent chattering
```

---

## 3.8 Safety System Design

### 3.8.1 Triple-Layer Protection Strategy

```mermaid
graph TB
    subgraph "Layer 1: Continuous Monitoring"
        L1[100ms Safety Checks<br/>High-Priority Task]
    end
    
    subgraph "Layer 2: Fault Detection"
        L2A[Over-Voltage Detection]
        L2B[Under-Voltage Detection]
        L2C[Over-Current Detection]
        L2D[Power-Present Detection]
    end
    
    subgraph "Layer 3: Protective Action"
        L3A[Immediate Relay Trip]
        L3B[Visual Alert - RGB LED]
        L3C[Display Alert]
        L3D[Cloud Notification]
    end
    
    L1 --> L2A
    L1 --> L2B
    L1 --> L2C
    L1 --> L2D
    L2A --> L3A
    L2B --> L3A
    L2C --> L3A
    L3A --> L3B
    L3A --> L3C
    L3A --> L3D
```

### 3.8.2 Hysteresis Implementation

To prevent relay chattering near threshold boundaries, hysteresis is implemented:

```
Trip Threshold:   216V (VOLTAGE_MIN)
Reset Threshold:  221V (VOLTAGE_MIN + HYSTERESIS)

State Transition:
- If NOT tripped: Trip when voltage < 216V
- If tripped: Reset only when voltage > 221V
```

**Hysteresis Diagram:**

```
Voltage (V)
    ^
250 |                    Normal Operation
    |    ════════════════════════════════
244 |    ← VOLTAGE_MAX (Trip threshold)
    |    ════════════════════════════════
240 |                    Safe Zone
    |
230 |
    |
221 |    ════════════════════════════════
    |    ← Reset threshold (MIN + HYST)
216 |    ════════════════════════════════
    |    ← VOLTAGE_MIN (Trip threshold)
    |
200 |              Under-Voltage Zone
    |
    +-----------------------------------> Time
    
    Hysteresis Band = 5V
```

---

## 3.9 Network Communication Architecture

### 3.9.1 IoT Communication Flow

```mermaid
sequenceDiagram
    participant ESP as ESP32
    participant WiFi as WiFi Module
    participant Router as WiFi Router
    participant Cloud as Blynk Cloud
    participant App as Mobile App
    
    ESP->>WiFi: Initialize WiFi
    WiFi->>Router: Connect Request
    Router-->>WiFi: Connection Established
    WiFi-->>ESP: WiFi Connected
    
    ESP->>Cloud: Authenticate (Auth Token)
    Cloud-->>ESP: Connection Accepted
    
    loop Every 2 seconds
        ESP->>Cloud: Publish Data (V, I, P)
        Cloud-->>App: Update Dashboard
    end
    
    alt Fault Detected
        ESP->>Cloud: Send Alert
        Cloud-->>App: Push Notification
    end
    
    alt User Reset Command
        App->>Cloud: Reset Button Pressed
        Cloud->>ESP: Reset Command
        ESP->>ESP: Handle Reset
        ESP->>Cloud: State Update
        Cloud-->>App: Confirm Reset
    end
```

### 3.9.2 Data Packet Structure

**Sensor Data Packet (Published every 2s):**
```
{
    "voltage": float,      // Volts (V)
    "current": float,      // Amperes (A)
    "power": float,        // Watts (W)
    "timestamp": long      // Unix timestamp
}
```

**State Update Packet:**
```
{
    "state": string,       // "NORMAL", "TRIP: reason", "OFFLINE"
    "timestamp": long
}
```

**Alert Packet:**
```
{
    "event": "safety_alert",
    "message": string,     // Fault reason
    "voltage": float,
    "current": float,
    "timestamp": long
}
```

---

## 3.10 Display Interface Design

### 3.10.1 Screen Layout Structure

```
┌────────────────────────────────┐
│ ACTIVE  [WiFi] [Blynk] [Batt] │ ← Status Bar (12px)
├────────────────────────────────┤
│                                │
│  VOLT                          │
│       223.5 V                  │ ← Voltage (Large)
│                                │
├────────────────────────────────┤
│ CURR          │  PWR           │
│ 12.50 A       │  2.79 kW       │ ← Current & Power
│                                │
└────────────────────────────────┘
```

### 3.10.2 Display State Machine

```mermaid
stateDiagram-v2
    [*] --> Startup
    Startup --> Normal : 3s delay
    Startup --> Offline : WiFi timeout
    
    Normal --> TripAlert : Fault detected
    Normal --> Offline : WiFi lost
    
    Offline --> Normal : WiFi restored
    Offline --> TripAlert : Fault detected
    
    TripAlert --> Normal : Reset successful
    TripAlert --> Offline : Reset + No WiFi
    
    note right of Startup
        Logo + Loading Bar
    end note
    
    note right of Normal
        V, I, P + Status Icons
    end note
    
    note right of Offline
        "OFFLINE MODE"
        + Sensor Data
    end note
    
    note right of TripAlert
        Inverted Screen
        "TRIPPED!"
        + Fault Reason
    end note
```

---

## 3.11 Timing and Scheduling

### 3.11.1 Task Timing Diagram

```
Time (ms)
    0   100  200  300  400  500  600  700  800  900  1000
    |    |    |    |    |    |    |    |    |    |    |
    
Safety Check (100ms)
    ▓    ▓    ▓    ▓    ▓    ▓    ▓    ▓    ▓    ▓    ▓
    
Sensor Read (500ms)
    ▓▓▓▓▓                   ▓▓▓▓▓                   ▓▓▓▓▓
    
Display Update (1000ms)
    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    
Blynk Publish (2000ms)
    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    
Legend:
    ▓ = Task executing
    | = Time marker
```

### 3.11.2 Priority Levels

| Task | Interval | Priority | Rationale |
|------|----------|----------|-----------|
| Safety Check | 100ms | CRITICAL | Fast fault detection |
| Sensor Read | 500ms | HIGH | Fresh data for safety |
| Display Update | 1000ms | MEDIUM | User feedback |
| Network Update | Continuous | MEDIUM | Non-blocking |
| Blynk Publish | 2000ms | LOW | Throttled by library |

---

## 3.12 Error Handling Strategy

### 3.12.1 Fault Recovery Flowchart

```mermaid
flowchart TD
    Start([Fault Detected]) --> Trip[Trip Relay Immediately]
    Trip --> Log[Log Fault Reason]
    Log --> Display[Display Alert]
    Display --> Cloud{Cloud<br/>Available?}
    
    Cloud -->|Yes| SendAlert[Send Alert to Cloud]
    Cloud -->|No| LocalOnly[Local Alert Only]
    
    SendAlert --> Monitor[Monitor Conditions]
    LocalOnly --> Monitor
    
    Monitor --> WaitReset{Reset<br/>Command?}
    WaitReset -->|No| Monitor
    WaitReset -->|Yes| CheckSafe{Conditions<br/>Safe?}
    
    CheckSafe -->|No| Reject[Reject Reset]
    Reject --> Monitor
    
    CheckSafe -->|Yes| ClearFault[Clear Fault]
    ClearFault --> ResetRelay[Reset Relay]
    ResetRelay --> Resume[Resume Normal Operation]
    Resume --> End([End])
```

### 3.12.2 Watchdog Protection

```cpp
// Implemented via yield() in main loop
void loop() {
    stateManager.update();
    yield();  // Prevent watchdog timeout
}
```

---

## 3.13 Testing Methodology

### 3.13.1 Test Categories

1. **Unit Testing**
   - Individual module functionality
   - Sensor reading accuracy
   - Safety logic correctness

2. **Integration Testing**
   - Module interaction
   - State transitions
   - Data flow validation

3. **System Testing**
   - End-to-end functionality
   - Real-world conditions
   - Performance under load

4. **Calibration Testing**
   - Sensor accuracy verification
   - Threshold validation
   - Noise immunity

### 3.13.2 Test Cases Summary

| Test ID | Category | Description | Expected Result |
|---------|----------|-------------|-----------------|
| TC-01 | Safety | Over-voltage trip | Relay trips at >250V |
| TC-02 | Safety | Under-voltage trip | Relay trips at <216V |
| TC-03 | Safety | Over-current trip | Relay trips at >30A |
| TC-04 | Safety | Power-off no trip | No trip at 0V |
| TC-05 | Sensor | Voltage accuracy | ±2% of multimeter |
| TC-06 | Sensor | Current accuracy | ±5% of clamp meter |
| TC-07 | Network | WiFi connection | Connects in <20s |
| TC-08 | Network | Blynk publish | Data updates every 2s |
| TC-09 | Display | Normal mode | Shows V, I, P correctly |
| TC-10 | Display | Trip alert | Shows inverted screen |

---

## 3.14 Summary

This chapter presented a comprehensive methodology for the Smart Energy Monitoring and Safety System, covering:

- **System Architecture**: Modular, layered design with clear separation of concerns
- **Algorithms**: Single-pass RMS calculation, EMA smoothing, and intelligent safety checks
- **State Machine**: Four-state FSM with robust transition logic
- **Safety Design**: Triple-layer protection with hysteresis and power-present detection
- **Network Integration**: IoT connectivity with Blynk cloud platform
- **Testing Strategy**: Multi-level testing from unit to system validation

The methodology emphasizes:
1. **Reliability**: Redundant safety checks and fault tolerance
2. **Accuracy**: Calibrated sensors with noise filtering
3. **Usability**: Clear visual feedback and remote monitoring
4. **Maintainability**: Modular code structure with comprehensive documentation

The implementation successfully balances theoretical rigor with practical engineering constraints, resulting in a robust and deployable energy monitoring solution.
