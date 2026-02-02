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

## Step 7: Notifications (Optional)

To receive alerts when the system trips:

1.  Go to **Templates** -> **Events**.
2.  Create a new event named `safety_alert`.
3.  Enable **Send Event to Timeline** and **Send Event to Notifications**.
4.  Go to **Notifications** tab in the console to configure email or push settings.

## Troubleshooting

- **Device Offline?** Check your WiFi credentials in `config.h`.
- **No Data?** Ensure the ESP32 is connected to the internet and the Auth Token is correct.
