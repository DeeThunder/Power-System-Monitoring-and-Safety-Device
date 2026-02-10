import os
import requests
import pandas as pd
import json
from pathlib import Path
import re
from datetime import datetime

# --- CONFIGURATION ---
TOOLS_DIR = Path(__file__).parent
DATA_DIR = TOOLS_DIR / "performance_data"
SECRETS_FILE = TOOLS_DIR / ".." / "include" / "secrets.h"

# Mapping of Google Sheet names to local CSV filenames and column structures
DATA_MAPPING = {
    "Accuracy": {
        "file": "accuracy.csv",
        "columns": ["DateTime", "Uptime(ms)", "Voltage(V)", "Current(A)", "Power(W)"]
    },
    "Latency": {
        "file": "latency.csv",
        "columns": ["DateTime", "Uptime(ms)", "SensorRead(us)", "BlynkTransmit(ms)", "TotalLatency(ms)"]
    },
    "TripResponse": {
        "file": "trip_response.csv",
        "columns": ["DateTime", "Uptime(ms)", "FaultDetect(us)", "RelayTrip(us)", "TotalResponse(ms)"]
    }
}

def get_secrets():
    """Extract Google Sheets URL from secrets.h."""
    if not SECRETS_FILE.exists():
        print(f"[ERROR] secrets.h not found at {SECRETS_FILE}")
        return None
    
    content = SECRETS_FILE.read_text()
    match = re.search(r'#define SECRET_GOOGLE_SHEETS_URL\s+"([^"]+)"', content)
    if match:
        return match.group(1)
    
    print("[ERROR] SECRET_GOOGLE_SHEETS_URL not found in secrets.h")
    return None

def fetch_sheet_data(url, sheet_name):
    """Fetch data from Google Apps Script via GET request."""
    print(f"Fetching {sheet_name} data from cloud...")
    try:
        response = requests.get(url, params={"sheet": sheet_name}, timeout=15)
        response.raise_for_status()
        result = response.json()
        
        if result.get("status") == "success":
            return result.get("data", [])
        else:
            print(f"[ERROR] Cloud error: {result.get('message')}")
            return None
    except Exception as e:
        print(f"[ERROR] Request failed: {e}")
        return None

def sync_sheet(url, sheet_name, mapping):
    """Sync a single sheet with its local CSV counterpart."""
    local_file = DATA_DIR / mapping["file"]
    
    # 1. Fetch cloud data
    cloud_rows = fetch_sheet_data(url, sheet_name)
    if not cloud_rows or len(cloud_rows) <= 1:
        print(f"No new data for {sheet_name}.")
        return

    # First row is usually headers in Sheets
    # Sheets structure: [Timestamp, Value1, Value2, ...]
    cloud_df = pd.DataFrame(cloud_rows[1:], columns=mapping["columns"])
    
    # Convert timestamp to standard string format
    # Note: Apps Script returns date strings like "2026-02-10T09:30:00.000Z"
    cloud_df["DateTime"] = pd.to_datetime(cloud_df["DateTime"]).dt.strftime('%Y-%m-%d %H:%M:%S')

    # 2. Load local data
    if local_file.exists():
        local_df = pd.read_csv(local_file)
        # Combine
        combined_df = pd.concat([local_df, cloud_df], ignore_index=True)
    else:
        combined_df = cloud_df

    # 3. Deduplicate based on DateTime and Uptime
    # This prevents duplicates if data was logged both via USB and Cloud
    initial_len = len(combined_df)
    combined_df = combined_df.drop_duplicates(subset=["DateTime", "Uptime(ms)"], keep="first")
    
    added_count = len(combined_df) - (len(local_df) if local_file.exists() else 0)
    
    # 4. Save back
    combined_df.sort_values(by="DateTime", inplace=True)
    combined_df.to_csv(local_file, index=False)
    
    print(f"Synced {sheet_name}: Added {added_count} new records. Total: {len(combined_df)}")

def main():
    print("=== Power System Data Synchronizer ===")
    
    url = get_secrets()
    if not url:
        return

    if not DATA_DIR.exists():
        DATA_DIR.mkdir(parents=True)

    for sheet, mapping in DATA_MAPPING.items():
        sync_sheet(url, sheet, mapping)
    
    print("\n[SUCCESS] Synchronization complete!")
    print(f"Data is ready for analysis in {DATA_DIR}")

if __name__ == "__main__":
    main()
