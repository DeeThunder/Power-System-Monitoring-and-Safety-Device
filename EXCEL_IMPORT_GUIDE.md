# How to Open CSV Files in Excel - Step by Step

## Method 1: Import Data (Recommended)

If double-clicking the CSV doesn't work properly, use Excel's import feature:

### Steps:
1. **Open Excel** (blank workbook)
2. **Go to Data tab** → Click "Get Data" or "From Text/CSV"
3. **Select your CSV file** (e.g., `latency_20260106_203015.csv`)
4. **Preview window appears**:
   - File Origin: `65001: Unicode (UTF-8)`
   - Delimiter: `Comma`
5. **Click "Load"**
6. **Data appears in proper columns!**

## Method 2: Text Import Wizard (Excel 2016 and older)

1. **Open Excel** (blank workbook)
2. **Go to Data tab** → Click "From Text"
3. **Select your CSV file**
4. **Text Import Wizard - Step 1**:
   - Choose "Delimited"
   - Click "Next"
5. **Text Import Wizard - Step 2**:
   - Check "Comma" as delimiter
   - Uncheck other delimiters
   - Click "Next"
6. **Text Import Wizard - Step 3**:
   - Column data format: "General"
   - Click "Finish"
7. **Data appears in proper columns!**

## Method 3: Change File Association (One-time setup)

If you want CSV files to always open correctly:

1. **Right-click any CSV file** → "Open with" → "Choose another app"
2. **Select Excel**
3. **Check "Always use this app"**
4. **Click OK**

## Expected Result

After importing correctly, you should see:

| DateTime | Uptime(ms) | SensorRead(us) | BlynkTransmit(ms) | TotalLatency(ms) |
|----------|------------|----------------|-------------------|------------------|
| 2026-01-06 20:30:15 | 1523 | 245123 | 15 | 260 |
| 2026-01-06 20:30:17 | 3045 | 243987 | 14 | 258 |
| 2026-01-06 20:30:19 | 4567 | 246234 | 16 | 262 |

Each value in its own cell, properly aligned in columns!

## Troubleshooting

### Problem: Still shows all data in one column
**Solution**: Your Excel might be using semicolon (;) as delimiter instead of comma (,)
- Go to Control Panel → Region → Additional Settings
- Change "List separator" to comma (,)
- Restart Excel

### Problem: DateTime shows as number
**Solution**: Format the DateTime column
- Select DateTime column
- Right-click → Format Cells
- Choose "Date" or "Custom"
- Format: `yyyy-mm-dd hh:mm:ss`

## Quick Test

After running the updated Python script, try opening the CSV file. It should now work perfectly in Excel with proper columns!
