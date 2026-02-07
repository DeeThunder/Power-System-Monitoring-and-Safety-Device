#!/usr/bin/env python3
"""
Performance Data Logger - PC Side

Captures CSV-formatted performance data from ESP32 via Serial
and saves to separate CSV files.

Usage:
    python performance_logger.py COM11 115200
    python performance_logger.py /dev/ttyUSB0 115200
"""

import serial
import sys
import os
import csv
from datetime import datetime

class PerformanceLogger:
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        
        # Create output directory
        self.output_dir = "performance_data"
        os.makedirs(self.output_dir, exist_ok=True)
        
        # CSV file handles and writers
        # Use fixed filenames (no timestamp) for continuous logging
        self.latency_path = f"{self.output_dir}/latency.csv"
        self.accuracy_path = f"{self.output_dir}/accuracy.csv"
        self.trip_path = f"{self.output_dir}/trip_response.csv"
        
        # Check if files exist to determine mode and whether to write headers
        latency_exists = os.path.exists(self.latency_path)
        accuracy_exists = os.path.exists(self.accuracy_path)
        trip_exists = os.path.exists(self.trip_path)
        
        # Open files in append mode if they exist, write mode if new
        # Open in text mode with newline='' for proper CSV handling
        self.latency_file = open(self.latency_path, "a" if latency_exists else "w", newline='', encoding='utf-8')
        self.accuracy_file = open(self.accuracy_path, "a" if accuracy_exists else "w", newline='', encoding='utf-8')
        self.trip_file = open(self.trip_path, "a" if trip_exists else "w", newline='', encoding='utf-8')
        
        # Create CSV writers
        self.latency_writer = csv.writer(self.latency_file)
        self.accuracy_writer = csv.writer(self.accuracy_file)
        self.trip_writer = csv.writer(self.trip_file)
        
        # Write headers only if creating new files
        if not latency_exists:
            self.latency_writer.writerow(["DateTime", "Uptime(ms)", "SensorRead(us)", "BlynkTransmit(ms)", "TotalLatency(ms)"])
        if not accuracy_exists:
            self.accuracy_writer.writerow(["DateTime", "Uptime(ms)", "Voltage(V)", "Current(A)", "Power(W)"])
        if not trip_exists:
            self.trip_writer.writerow(["DateTime", "Uptime(ms)", "FaultDetect(us)", "RelayTrip(us)", "TotalResponse(ms)"])
        
        # Flush to ensure headers are written (if new files)
        self.latency_file.flush()
        self.accuracy_file.flush()
        self.trip_file.flush()
        
        print(f"[*] Saving data to: {self.output_dir}/")
        if latency_exists or accuracy_exists or trip_exists:
            print(f"   - Appending to existing CSV files")
        else:
            print(f"   - Creating new CSV files")
        print(f"   - latency.csv {'(appending)' if latency_exists else '(new)'}")
        print(f"   - accuracy.csv {'(appending)' if accuracy_exists else '(new)'}")
        print(f"   - trip_response.csv {'(appending)' if trip_exists else '(new)'}")
    
    def connect(self):
        """Connect to ESP32 via Serial"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"[OK] Connected to {self.port} at {self.baudrate} baud")
            return True
        except Exception as e:
            print(f"[ERROR] Failed to connect: {e}")
            return False
    
    def process_line(self, line):
        """Process incoming CSV line and route to appropriate file"""
        line = line.strip()
        if not line:
            return
        
        # Get current datetime for this data point
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # Check for performance data markers
        if line.startswith("[LATENCY],"):
            data = line.replace("[LATENCY],", "")
            # Split CSV data
            values = data.split(',')
            if len(values) >= 4:
                # Write row: DateTime, Uptime, SensorRead, BlynkTransmit, TotalLatency
                self.latency_writer.writerow([now] + values)
                self.latency_file.flush()
                print(f"[LATENCY] {now}, {data}")
        
        elif line.startswith("[ACCURACY],"):
            data = line.replace("[ACCURACY],", "")
            # Split CSV data
            values = data.split(',')
            if len(values) >= 4:
                # Write row: DateTime, Uptime, Voltage, Current, Power
                self.accuracy_writer.writerow([now] + values)
                self.accuracy_file.flush()
                print(f"[ACCURACY] {now}, {data}")
        
        elif line.startswith("[TRIP],"):
            data = line.replace("[TRIP],", "")
            # Split CSV data
            values = data.split(',')
            if len(values) >= 4:
                # Write row: DateTime, Uptime, FaultDetect, RelayTrip, TotalResponse
                self.trip_writer.writerow([now] + values)
                self.trip_file.flush()
                print(f"[TRIP] {now}, {data}")
        
        else:
            # Regular debug output - just print
            print(line)
    
    def run(self):
        """Main loop - read Serial and save to CSV"""
        print("\n[*] Logging started... Press Ctrl+C to stop\n")
        
        try:
            while True:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore')
                    self.process_line(line)
        
        except KeyboardInterrupt:
            print("\n\n[*] Logging stopped by user")
        
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Close files and Serial connection"""
        print("\n[*] Closing files...")
        self.latency_file.close()
        self.accuracy_file.close()
        self.trip_file.close()
        
        if self.ser:
            self.ser.close()
        
        print("[OK] Done!")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python performance_logger.py <PORT> [BAUDRATE]")
        print("Example: python performance_logger.py COM3 115200")
        print("Example: python performance_logger.py /dev/ttyUSB0 115200")
        sys.exit(1)
    
    port = sys.argv[1]
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
    
    logger = PerformanceLogger(port, baudrate)
    
    if logger.connect():
        logger.run()
