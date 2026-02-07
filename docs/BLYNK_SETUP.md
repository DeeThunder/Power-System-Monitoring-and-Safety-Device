# Blynk Setup Guide for Smart Energy Monitor

This guide will help you set up the Blynk IoT dashboard to monitor your Energy System remotely.

## Prerequisites

1.  **Blynk Account:** Create an account at [blynk.io](https://blynk.io) or log in.
2.  **Blynk Mobile App:** Download the Blynk IoT app on your smartphone (iOS or Android).

## Step 1: Create a New Template

1.  Log in to the **Blynk Console** (Web Dashboard).
2.  Go to **Templates** -> **+ New Template**.
3.  **Name:** `Energy Monitor`
4.  **Hardware:** `ESP32`
5.  **Connection Type:** `WiFi`
6.  Click **Done**.

## Step 2: Configure Datastreams

You need to set up "Datastreams" to link the code to the dashboard widgets. Go to the **Datastreams** tab in your new template and add the following:

| Name             | Pin  | Data Type | Units     | Min | Max  | Description                         |
| :--------------- | :--- | :-------- | :-------- | :-- | :--- | :---------------------------------- |
| **Voltage**      | `V0` | Double    | Volts (V) | 0   | 300  | AC Voltage reading                  |
| **Current**      | `V1` | Double    | Amps (A)  | 0   | 30   | AC Current reading                  |
| **Power**        | `V2` | Double    | Watts (W) | 0   | 5000 | Total Power consumption             |
| **System State** | `V3` | String    | None      | -   | -    | Current status (e.g., Normal, Trip) |
| **Reset Button** | `V4` | Integer   | None      | 0   | 1    | Button to reset system after trip   |
| **Manual Switch**| `V5` | Integer   | None      | 0   | 1    | Manual ON/OFF control switch        |
| **Master Override** | `V6` | Integer | None    | 0   | 1    | ⚠️ Bypass safety protection (DANGER) |

## Step 3: Create the Web Dashboard (Optional)

1.  Go to the **Web Dashboard** tab.
2.  Drag and drop **Label** or **Gauge** widgets for Voltage, Current, and Power.
3.  Assign them to the datastreams created in Step 2 (`V0`, `V1`, `V2`).
4.  Add a **Label** widget for System State (`V3`).
5.  Add a **Switch** or **Button** widget for Reset (`V4`).

## Step 4: Create the Mobile Dashboard

1.  Open the **Blynk App** on your phone.
2.  Enable **Developer Mode** (if needed) and find your `Energy Monitor` template.
3.  Tap on the template to edit the dashboard.
4.  Add the following widgets:

    - **Gauge** (or Labeled Value) -> Select Datastream: **Voltage (V0)**
    - **Gauge** (or Labeled Value) -> Select Datastream: **Current (V1)**
    - **Gauge** (or Labeled Value) -> Select Datastream: **Power (V2)**
    - **Value Display** -> Select Datastream: **System State (V3)**
    - **Button** -> Select Datastream: **Reset Button (V4)**
      - **Mode:** `Push` (Important!)
    - **Switch** → Select Datastream: **Manual Switch (V5)**
      - **Mode:** `Switch` (toggle ON/OFF)
      - **Label:** "Manual Control"
      - **Description:** "Turn load ON/OFF manually"
    - **Switch** → Select Datastream: **Master Override (V6)**
      - **Mode:** `Switch` (toggle ON/OFF)
      - **Label:** "⚠️ MASTER OVERRIDE"
      - **Description:** "DANGER: Bypasses ALL safety protection!"
      - **Color:** Red (to indicate danger)
      - **⚠️ WARNING**: Only use during maintenance/testing when conditions are known

## ⚠️ Master Override Safety Warning

The Master Override feature **BYPASSES ALL SAFETY PROTECTION** and allows the system to operate outside safe limits. This is **EXTREMELY DANGEROUS** and should only be used:

- During maintenance when you need to test specific conditions
- When you understand the electrical parameters and accept the risks
- For temporary operation while troubleshooting

**When Override is Active:**
- ✅ System operates despite overvoltage/undervoltage/overcurrent
- ✅ LED turns **YELLOW** (solid) to indicate override active
- ✅ Periodic warnings sent every 30 minutes
- ⚠️ **EXCEPTION**: Extreme overcurrent (>150% of max) will still trip for fire prevention
- ⚠️ **RISK**: Equipment damage, fire hazard, electrical shock

**Always disable override when not needed!**

## Step 5: Get Credentials

1.  Save your template.
2.  Go to **Search** (Magnifying glass) -> **+ New Device**.
3.  Select **From Template** -> Choose `Energy Monitor`.
4.  Click **Create**.
5.  You will see a screen with `BLYNK_TEMPLATE_ID`, `BLYNK_TEMPLATE_NAME`, and `BLYNK_AUTH_TOKEN`.
6.  **Copy these values.**

## Step 6: Update the Code

1.  Open `include/config.h` in your project.
2.  Replace the placeholder values with your new credentials:

```cpp
#define BLYNK_TEMPLATE_ID     "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME   "Energy Monitor"
#define BLYNK_AUTH_TOKEN      "Your_Auth_Token_Here"
```

3.  **Upload** the code to your ESP32.

## Step 7: Configure Push Notifications

To receive push notifications when the system trips, you MUST configure the event in the Blynk Console:

### 7.1 Create Event in Blynk Console

1.  Log into [Blynk Console](https://blynk.cloud/)
2.  Navigate to **Templates** → Select your template → **Events** tab
3.  Click **"+ Create Event"** (or edit existing `safety_alert` if it exists)
4.  Configure the event:
    - **Event Code**: `safety_alert` (must match exactly)
    - **Event Name**: "Safety Alert" (display name)
    - **Description**: "System trip protection triggered"
    - **Color**: Red (for visibility)
    - **Send push notification**: ✅ **CHECK THIS BOX** (critical!)
    - **Recipients**: Select "Device Owner" or "All Users"
    - **Limit Period**: Set to "No Limit" (for testing) or "1 hour" (for production)
    - **Event Counter**: Leave at default or set to 1
5.  Click **Save**

### 7.2 Enable Notifications in Mobile App

1.  Open the **Blynk IoT** app on your phone
2.  Go to your device
3.  Tap the **Settings** icon (gear icon)
4.  Ensure **"Enable Notifications"** is turned ON
5.  Check your phone's system settings:
    - Go to **Settings** → **Notifications** → **Blynk**
    - Ensure notifications are **Allowed**
    - Turn OFF "Do Not Disturb" mode (if active)

### 7.3 Test Notifications

1.  Trigger a trip event (temporarily lower `VOLTAGE_MAX` in `config.h` to 200V, or disconnect power)
2.  You should receive:
    - **Push notification** on your phone
    - **Timeline entry** in the Blynk app
    - **Email** (if email notifications are also enabled)
3.  If you only see the timeline entry but no push notification, review the troubleshooting section below

## Troubleshooting

### Device Issues
- **Device Offline?** Check your WiFi credentials in `config.h`.
- **No Data?** Ensure the ESP32 is connected to the internet and the Auth Token is correct.
- **Blynk not connecting?** Check serial monitor for connection errors.

### Notification Issues
- **No push notifications but timeline shows events?**
  - Verify "Send push notification" is checked in the event configuration
  - Check phone notification permissions for Blynk app
  - Disable "Do Not Disturb" mode on your phone
  - Try uninstalling and reinstalling the Blynk app
  - Check if you've exceeded the daily notification limit (100 events/day on free plan)

- **Notifications delayed?**
  - Blynk has a 15-second minimum interval between notifications
  - Check the "Limit Period" setting in your event configuration

- **Still not working?**
  - Try enabling **Email notifications** instead (more reliable)
  - Consider implementing a **Telegram Bot** as an alternative notification method
  - Check Blynk community forums for known issues

### Alternative Notification Methods

If Blynk push notifications are unreliable, consider these alternatives:

1.  **Email Notifications** (via Blynk): Enable in the same Events tab, more reliable than push
2.  **Telegram Bot**: Free, reliable, works even when app is closed (requires code modification)
3.  **SMS via Twilio**: Very reliable but requires paid API subscription
4.  **IFTTT Webhooks**: Can trigger notifications to multiple platforms

