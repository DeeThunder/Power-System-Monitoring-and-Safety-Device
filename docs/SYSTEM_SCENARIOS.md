# Complete System Behavior Scenarios - Updated

## Overview

This document covers **all possible scenarios** with the updated system behavior including power notifications, manual control, and offline mode.

---

## Scenario Matrix

| # | Condition | State | Relay | LED | Notifications |
|---|-----------|-------|-------|-----|---------------|
| 1 | Boot + No Mains | NORMAL | OFF | Green | "POWER OUTAGE" (after 3s) |
| 2 | Boot + Mains Good + WiFi | NORMAL | ON | Green | None |
| 3 | Boot + Mains Good + No WiFi | OFFLINE_MODE | ON | Blue | None |
| 4 | Normal Operation | NORMAL | ON | Green | None |
| 5 | Overvoltage (255V) | TRIP | OFF | Red | "OVER VOLTAGE" |
| 6 | Undervoltage (180V) | TRIP | OFF | Red | "UNDER VOLTAGE" |
| 7 | Overcurrent (35A) | TRIP | OFF | Red | "OVER CURRENT" |
| 8 | Power Outage During Normal | NORMAL | Current | Green | "POWER OUTAGE" (after 3s) |
| 9 | Power Restored | NORMAL | Depends | Green | "POWER RESTORED" (after 3s) |
| 10 | Manual Switch OFF | NORMAL | OFF | Orange Blink | "LOAD SWITCHED OFF" |
| 11 | Manual Switch ON | NORMAL | ON | Green | None |
| 12 | WiFi Lost During Normal | OFFLINE_MODE | Current | Blue | None |
| 13 | WiFi Restored | NORMAL | Current | Green | None |
| 14 | Manual OFF + Power Outage | NORMAL | OFF | Orange Blink | Both notifications |
| 15 | Fault + WiFi Lost | TRIP | OFF | Red | Fault notification only |

---

## Detailed Scenarios

### Scenario 1: System Boot with No Mains Power

**Initial Conditions**:
- ESP32 powered on (USB/backup power)
- Mains voltage = 0V
- No load connected

**Timeline**:
```
T+0s:  System boots
       State: BOOT
       LED: Yellow blinking
       Relay: OFF

T+3s:  Boot screen complete
       WiFi connecting...

T+10s: WiFi connected
       State: BOOT → NORMAL
       LED: Yellow → Green
       powerWasPresent_ = TRUE (initial)

T+11s: First voltage reading: 0V
       powerPresent = FALSE
       Power state change detected: TRUE → FALSE
       Start debounce timer

T+14s: Still 0V (debounce complete)
       Notification: "POWER OUTAGE: Mains voltage lost"
       powerWasPresent_ = FALSE
       State: NORMAL (no change)
       LED: Green (no change)
       Relay: OFF (no change)
```

**Result**:
- ✅ State: NORMAL
- ✅ Relay: OFF
- ✅ LED: Green (solid)
- ✅ Notification: "POWER OUTAGE"
- ✅ System ready, waiting for power

---

### Scenario 2: System Boot with Good Mains + WiFi

**Initial Conditions**:
- ESP32 powered on
- Mains voltage = 220V
- WiFi available

**Timeline**:
```
T+0s:  System boots
       State: BOOT
       LED: Yellow blinking

T+3s:  Boot screen complete
       WiFi connecting...

T+8s:  WiFi connected
       State: BOOT → NORMAL
       LED: Yellow → Green
       Relay: OFF → ON (energize)

T+9s:  First voltage reading: 220V
       powerPresent = TRUE
       powerWasPresent_ = TRUE (initial)
       No power state change
       No notification

T+10s: Safety check: PASS
       All readings normal
       State: NORMAL
       Relay: ON
       LED: Green
```

**Result**:
- ✅ State: NORMAL
- ✅ Relay: ON
- ✅ LED: Green (solid)
- ✅ Notification: None
- ✅ System operating normally

---

### Scenario 3: System Boot with Good Mains + No WiFi

**Initial Conditions**:
- ESP32 powered on
- Mains voltage = 220V
- WiFi not available

**Timeline**:
```
T+0s:  System boots
       State: BOOT
       LED: Yellow blinking

T+3s:  Boot screen complete
       WiFi connecting...

T+33s: WiFi timeout (30 seconds)
       State: BOOT → OFFLINE_MODE
       LED: Yellow → Blue
       Relay: OFF → ON (energize)

T+34s: First voltage reading: 220V
       powerPresent = TRUE
       powerWasPresent_ = TRUE
       No power state change
       No notification

T+35s: Safety check: PASS
       State: OFFLINE_MODE
       Relay: ON
       LED: Blue
       Local operation active
```

**Result**:
- ✅ State: OFFLINE_MODE
- ✅ Relay: ON
- ✅ LED: Blue (solid)
- ✅ Notification: None (no internet)
- ✅ Full local functionality

---

### Scenario 4: Normal Operation

**Conditions**:
- Mains: 220V (good)
- Current: 5A (normal)
- WiFi: Connected
- User: No action

**Behavior**:
```
Continuous monitoring:
  Voltage: 220V ✓
  Current: 5A ✓
  Safety: PASS ✓

State: NORMAL
Relay: ON
LED: Green (solid)
Notifications: None
Display: Shows V, I, P
Blynk: Updates every 1 second
```

**Result**:
- ✅ Everything normal
- ✅ No notifications
- ✅ Continuous monitoring

---

### Scenario 5: Overvoltage Fault

**Initial Conditions**:
- Normal operation (220V)
- Voltage suddenly rises to 255V

**Timeline**:
```
T+0s:  Voltage: 220V → 255V
       Safety check: checkOverVoltage(255V)
       255V > 249V = TRUE
       Fault detected!

T+0s:  State: NORMAL → TRIP_PROTECTION
       Relay: ON → OFF (trip)
       LED: Green → Red
       Notification: "OVER VOLTAGE"
       Blynk switch: Synced to OFF

T+1s:  Voltage still 255V
       State: TRIP_PROTECTION (stays)
       Relay: OFF (stays)
       No additional notifications
```

**Result**:
- ✅ State: TRIP_PROTECTION
- ✅ Relay: OFF (tripped)
- ✅ LED: Red (solid)
- ✅ Notification: "OVER VOLTAGE" (once)
- ✅ Requires manual reset

---

### Scenario 6: Undervoltage Fault

**Initial Conditions**:
- Normal operation (220V)
- Voltage drops to 180V (brownout)

**Timeline**:
```
T+0s:  Voltage: 220V → 180V
       Safety check:
         powerPresent = TRUE (180V >= 100V)
         checkUnderVoltage(180V)
         180V < 200V = TRUE
       Fault detected!

T+0s:  State: NORMAL → TRIP_PROTECTION
       Relay: ON → OFF (trip)
       LED: Green → Red
       Notification: "UNDER VOLTAGE"

T+1s:  Voltage still 180V
       State: TRIP_PROTECTION (stays)
       No additional notifications
```

**Result**:
- ✅ State: TRIP_PROTECTION
- ✅ Relay: OFF (tripped)
- ✅ LED: Red (solid)
- ✅ Notification: "UNDER VOLTAGE" (once)
- ✅ Requires manual reset

**Important**: If voltage drops below 100V, it's treated as power outage, not undervoltage!

---

### Scenario 7: Overcurrent Fault

**Initial Conditions**:
- Normal operation (220V, 5A)
- Current suddenly rises to 35A

**Timeline**:
```
T+0s:  Current: 5A → 35A
       Safety check: checkOverCurrent(35A)
       35A > 30A = TRUE
       Fault detected!

T+0s:  State: NORMAL → TRIP_PROTECTION
       Relay: ON → OFF (trip)
       LED: Green → Red
       Notification: "OVER CURRENT"

T+1s:  Current drops to 0A (relay OFF)
       State: TRIP_PROTECTION (stays)
       No additional notifications
```

**Result**:
- ✅ State: TRIP_PROTECTION
- ✅ Relay: OFF (tripped)
- ✅ LED: Red (solid)
- ✅ Notification: "OVER CURRENT" (once)
- ✅ Requires manual reset

---

### Scenario 8: Power Outage During Normal Operation

**Initial Conditions**:
- Normal operation (220V, 5A)
- Relay ON, LED Green
- Power suddenly disconnected

**Timeline**:
```
T+0s:  Voltage: 220V → 0V (instant)
       powerPresent: TRUE → FALSE
       Power state change detected
       Start 3-second debounce

T+1s:  Voltage: 0V (still)
       Debounce in progress...

T+2s:  Voltage: 0V (still)
       Debounce in progress...

T+3s:  Voltage: 0V (debounce complete)
       Notification: "POWER OUTAGE: Mains voltage lost"
       powerWasPresent_ = FALSE
       State: NORMAL (no change)
       Relay: Maintains position (ON)
       LED: Green (no change)

T+4s:  Voltage: 0V
       checkSafety returns TRUE (safe - power just off)
       No trip
       State: NORMAL
```

**Result**:
- ✅ State: NORMAL (no change)
- ✅ Relay: Maintains position
- ✅ LED: Green (no change)
- ✅ Notification: "POWER OUTAGE" (after 3s)
- ✅ No fault trip

---

### Scenario 9: Power Restored After Outage

**Initial Conditions**:
- Power was out (0V)
- System in NORMAL state
- Power reconnected

**Timeline**:
```
T+0s:  Voltage: 0V → 220V (instant)
       powerPresent: FALSE → TRUE
       Power state change detected
       Start 3-second debounce

T+1s:  Voltage: 220V (still)
       Debounce in progress...

T+2s:  Voltage: 220V (still)
       Debounce in progress...

T+3s:  Voltage: 220V (debounce complete)
       Notification: "POWER RESTORED: Mains voltage detected"
       powerWasPresent_ = TRUE
       Safety check: PASS (220V is good)
       State: NORMAL (no change)
       Relay: ON (energize if was ON before)
       LED: Green

T+4s:  Normal operation resumes
       All monitoring active
```

**Result**:
- ✅ State: NORMAL
- ✅ Relay: ON (if was ON before outage)
- ✅ LED: Green
- ✅ Notification: "POWER RESTORED" (after 3s)
- ✅ Normal operation resumed

---

### Scenario 10: Manual Switch OFF via Blynk

**Initial Conditions**:
- Normal operation (220V, 5A)
- Relay ON, LED Green
- User presses OFF in Blynk app

**Timeline**:
```
T+0s:  Blynk command received: VPIN_MANUAL_SWITCH = 0
       handleManualSwitch(false) called

T+0s:  Relay: ON → OFF (de-energize)
       LED: Green → Orange (blinking)
       Notification: "LOAD SWITCHED OFF: Manual control via Blynk"
       State: NORMAL (no change)

T+1s:  Voltage: 220V (still monitored)
       Current: 0A (relay OFF)
       Safety: Still active
       State: NORMAL
       LED: Orange blinking
       Relay: OFF
```

**Result**:
- ✅ State: NORMAL (no change)
- ✅ Relay: OFF
- ✅ LED: Orange (blinking)
- ✅ Notification: "LOAD SWITCHED OFF"
- ✅ Safety monitoring continues

---

### Scenario 11: Manual Switch ON via Blynk

**Initial Conditions**:
- Load manually OFF (orange blinking)
- Mains good (220V)
- User presses ON in Blynk app

**Timeline**:
```
T+0s:  Blynk command received: VPIN_MANUAL_SWITCH = 1
       handleManualSwitch(true) called

T+0s:  Safety check performed:
       Voltage: 220V ✓
       Current: 0A ✓
       checkSafety: PASS

T+0s:  Relay: OFF → ON (energize)
       LED: Orange blinking → Green (solid)
       Stop blinking
       State: NORMAL (no change)
       No notification (normal operation resumed)

T+1s:  Current: 5A (load drawing power)
       Normal operation
```

**Result**:
- ✅ State: NORMAL
- ✅ Relay: ON
- ✅ LED: Green (solid)
- ✅ Notification: None
- ✅ Normal operation

**If Unsafe**:
```
T+0s:  Safety check: FAIL (e.g., voltage = 255V)
       Relay: Stays OFF
       Blynk switch: Synced back to OFF
       LED: Stays orange blinking
       State: NORMAL (no change)
       User cannot turn ON until safe
```

---

### Scenario 12: WiFi Lost During Normal Operation

**Initial Conditions**:
- Normal operation (220V, 5A)
- WiFi connected
- WiFi suddenly disconnects

**Timeline**:
```
T+0s:  WiFi connection lost
       network_.isWiFiConnected() = FALSE

T+0s:  State: NORMAL → OFFLINE_MODE
       LED: Green → Blue
       Relay: Maintains state (ON)
       No notification (no internet to send)

T+1s:  Local operation continues:
       Voltage monitoring: Active
       Current monitoring: Active
       Safety checks: Active
       Display: Active
       Blynk: Disconnected

T+2s:  If fault occurs:
       Safety will still trip
       LED: Blue → Red
       Relay: OFF
       No notification (no internet)
```

**Result**:
- ✅ State: OFFLINE_MODE
- ✅ Relay: Maintains state
- ✅ LED: Blue (solid)
- ✅ Notification: None (no internet)
- ✅ Full local functionality

---

### Scenario 13: WiFi Restored

**Initial Conditions**:
- System in OFFLINE_MODE
- WiFi reconnects

**Timeline**:
```
T+0s:  WiFi connection restored
       network_.isWiFiConnected() = TRUE

T+0s:  State: OFFLINE_MODE → NORMAL
       LED: Blue → Green
       Relay: Maintains state
       Blynk: Reconnecting...

T+2s:  Blynk connected
       Data sync to Blynk
       Remote control available
       Normal operation
```

**Result**:
- ✅ State: NORMAL
- ✅ Relay: Maintains state
- ✅ LED: Green
- ✅ Notification: None
- ✅ Online operation resumed

---

### Scenario 14: Manual OFF + Power Outage (Combined)

**Initial Conditions**:
- User manually switched OFF (orange blinking)
- Power then goes out

**Timeline**:
```
T+0s:  User presses OFF
       Relay: OFF
       LED: Orange blinking
       State: NORMAL
       Notification: "LOAD SWITCHED OFF: Manual control via Blynk"

T+10s: Power goes out (220V → 0V)
       powerPresent: TRUE → FALSE
       Start debounce

T+13s: Still 0V (debounce complete)
       Notification: "POWER OUTAGE: Mains voltage lost"
       State: NORMAL (no change)
       Relay: OFF (already OFF)
       LED: Orange blinking (no change)

T+20s: Power restored (0V → 220V)
       powerPresent: FALSE → TRUE
       Start debounce

T+23s: Still 220V (debounce complete)
       Notification: "POWER RESTORED: Mains voltage detected"
       State: NORMAL (no change)
       Relay: OFF (still manually OFF)
       LED: Orange blinking (still)
```

**Result**:
- ✅ State: NORMAL (throughout)
- ✅ Relay: OFF (manual control maintained)
- ✅ LED: Orange blinking (throughout)
- ✅ Notifications: All 3 sent
  1. "LOAD SWITCHED OFF"
  2. "POWER OUTAGE"
  3. "POWER RESTORED"

**Key Point**: Manual OFF state is preserved through power outage/restoration

---

### Scenario 15: Fault + WiFi Lost (Combined)

**Initial Conditions**:
- Normal operation
- Overvoltage occurs (255V)
- WiFi also disconnects

**Timeline**:
```
T+0s:  Voltage: 220V → 255V
       Fault detected: OVER VOLTAGE
       State: NORMAL → TRIP_PROTECTION
       Relay: OFF
       LED: Red
       Notification: "OVER VOLTAGE" (sent before WiFi lost)

T+1s:  WiFi disconnects
       State: TRIP_PROTECTION (no change)
       LED: Red (no change)
       Relay: OFF (no change)
       Note: Cannot transition to OFFLINE_MODE (fault takes priority)

T+10s: WiFi reconnects
       State: TRIP_PROTECTION (still)
       LED: Red (still)
       Relay: OFF (still)
       User must reset to clear fault

T+15s: User presses RESET
       Safety check: Voltage still 255V
       Reset DENIED (unsafe)
       State: TRIP_PROTECTION (stays)
```

**Result**:
- ✅ State: TRIP_PROTECTION (fault priority)
- ✅ Relay: OFF
- ✅ LED: Red
- ✅ Notification: "OVER VOLTAGE" (if sent before WiFi lost)
- ✅ Fault must be cleared before normal operation

---

## State Priority

When multiple conditions occur simultaneously:

```
1. TRIP_PROTECTION (highest priority)
   - Fault conditions always take precedence
   - Cannot be overridden by WiFi state

2. Manual Control
   - User can turn OFF in any state
   - User can turn ON only if safe

3. OFFLINE_MODE
   - Only if no faults present
   - Only if WiFi disconnected

4. NORMAL
   - Default operational state
```

---

## Notification Frequency Summary

| Notification | Frequency | Debounce |
|--------------|-----------|----------|
| POWER OUTAGE | Once per outage | 3 seconds |
| POWER RESTORED | Once per restoration | 3 seconds |
| OVER VOLTAGE | Once per trip | None |
| UNDER VOLTAGE | Once per trip | None |
| OVER CURRENT | Once per trip | None |
| LOAD SWITCHED OFF | Once per manual OFF | None |

**Key**: All notifications sent **once per event**, not continuously.

---

## LED Behavior Summary

| LED Color | Blinking | State | Meaning |
|-----------|----------|-------|---------|
| Yellow | Yes | BOOT | Starting up |
| Green | No | NORMAL | Normal operation, online |
| Orange | Yes | NORMAL (manual OFF) | Load manually switched OFF |
| Red | No | TRIP_PROTECTION | Fault condition |
| Blue | No | OFFLINE_MODE | No internet, local operation |

---

## Summary

This document covers **all 15 major scenarios** including:
- ✅ Power states (on, off, outage, restoration)
- ✅ Fault conditions (overvoltage, undervoltage, overcurrent)
- ✅ Manual control (ON, OFF)
- ✅ Internet connectivity (online, offline)
- ✅ Combined scenarios

**System behavior is consistent, predictable, and safe in all cases.**
