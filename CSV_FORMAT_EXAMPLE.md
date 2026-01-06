# CSV Format Example - After Update

## New CSV Format with Real DateTime

### latency_YYYYMMDD_HHMMSS.csv
```csv
DateTime,Uptime(ms),SensorRead(us),BlynkTransmit(ms),TotalLatency(ms)
2026-01-06 20:20:15,1523,245123,15,260
2026-01-06 20:20:17,3045,243987,14,258
2026-01-06 20:20:19,4567,246234,16,262
2026-01-06 20:20:21,6089,244567,15,259
2026-01-06 20:20:23,7611,245890,14,259
```

### accuracy_YYYYMMDD_HHMMSS.csv
```csv
DateTime,Uptime(ms),Voltage(V),Current(A),Power(W)
2026-01-06 20:20:25,10234,223.45,5.23,1168.62
2026-01-06 20:20:35,20456,223.51,5.21,1164.49
2026-01-06 20:20:45,30678,223.48,5.22,1166.57
2026-01-06 20:20:55,40890,223.52,5.24,1170.45
```

### trip_response_YYYYMMDD_HHMMSS.csv
```csv
DateTime,Uptime(ms),FaultDetect(us),RelayTrip(us),TotalResponse(ms)
2026-01-06 20:21:05,45123,125,85,0.21
2026-01-06 20:21:30,70456,130,90,0.22
```

## Benefits of This Format

### ✅ Real DateTime Column
- **Exact time** when each measurement was taken
- **Easy to correlate** with external events (e.g., "voltage dropped at 8:30 PM")
- **Professional presentation** in reports

### ✅ Uptime Column
- **System runtime** since ESP32 boot
- **Useful for debugging** (e.g., "issue started after 2 hours")
- **Relative timing** between events

### ✅ Proper Tabular Format
- **Opens perfectly in Excel** - Each column properly aligned
- **Sortable** - Sort by DateTime or any other column
- **Filterable** - Filter data by time range
- **Graph-ready** - Select columns and create charts instantly

## How to Use in Excel

1. **Open CSV file** - Double-click the file
2. **Data appears in columns** - Each measurement in its own column
3. **Create graphs**:
   - Select DateTime and TotalLatency columns
   - Insert → Line Chart
   - See latency over actual time!
4. **Calculate statistics**:
   ```excel
   =AVERAGE(E:E)  // Average latency
   =MAX(E:E)      // Maximum latency
   =MIN(E:E)      // Minimum latency
   ```

## Sample Analysis

### Find peak latency time:
1. Sort by "TotalLatency(ms)" column (highest to lowest)
2. Look at "DateTime" column to see when it occurred

### Compare morning vs evening performance:
1. Filter "DateTime" column for morning hours (08:00-12:00)
2. Calculate average
3. Filter for evening hours (18:00-22:00)
4. Compare averages

### Track voltage stability over time:
1. Open accuracy CSV
2. Create line chart with DateTime (X-axis) and Voltage (Y-axis)
3. Visually see any voltage fluctuations

## Next Steps

Run the updated logger:
```bash
python tools/performance_logger.py COM11 115200
```

The CSV files will now have:
- ✅ Real date/time stamps
- ✅ Proper tabular format
- ✅ Ready for Excel analysis
- ✅ Professional presentation quality
