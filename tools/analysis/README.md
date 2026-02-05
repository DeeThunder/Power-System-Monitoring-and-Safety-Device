# Performance Analysis Scripts

This folder contains Python scripts for analyzing the performance data collected from the Smart Energy Monitoring System.

## Data Files

The analysis scripts process the following CSV files from the performance_data directory:
- `../../performance_data/latency.csv` - Transmission latency measurements
- `../../performance_data/accuracy.csv` - Voltage, current, and power accuracy measurements
- `../../performance_data/trip_response.csv` - Safety trip response time data

## Analysis Scripts

### 1. `latency_analysis.py`
Analyzes transmission latency metrics:
- Sensor read time distribution
- Blynk transmission delays
- Total end-to-end latency
- Temporal trends and anomaly detection
- Component breakdown and bottleneck identification

### 2. `accuracy_analysis.py`
Evaluates measurement accuracy:
- Voltage sensor calibration validation
- Current sensor accuracy assessment
- Statistical error metrics (MAE, RMSE, MAPE)
- Noise and stability analysis
- Sensor drift over time

### 3. `trip_response_analysis.py`
Assesses safety protection performance:
- Fault detection time analysis
- Relay actuation time statistics
- Total response time distribution
- Safety compliance validation
- Reliability metrics

### 4. `integrated_analysis.py`
Performs multi-metric correlation analysis:
- Relationships between latency, accuracy, and response time
- System-wide performance assessment
- Comprehensive reporting

### 5. `generate_report.py`
Generates comprehensive performance report:
- Executive summary with key findings
- Detailed analysis results
- Visualizations and charts
- Recommendations for optimization

## Requirements

Install required Python packages:
```bash
pip install pandas numpy matplotlib seaborn scipy scikit-learn
```

Or use the provided requirements file:
```bash
pip install -r requirements.txt
```

## Usage

**Note: Do not run these scripts yet until sufficient data is collected.**

Once you have adequate data:

```bash
# Run individual analyses
python latency_analysis.py
python accuracy_analysis.py
python trip_response_analysis.py

# Run integrated analysis
python integrated_analysis.py

# Generate comprehensive report
python generate_report.py
```

## Output

Analysis results will be saved in the `results/` subdirectory:
- `results/figures/` - Generated plots and visualizations
- `results/tables/` - Statistical summary tables
- `results/reports/` - Comprehensive analysis reports

## Data Collection Guidelines

For robust analysis, collect:
- **Latency data**: At least 1000 samples across different conditions
- **Accuracy data**: Reference measurements with calibrated instruments
- **Trip response data**: Multiple fault scenarios (over-voltage, under-voltage, over-current)

## Analysis Plan

Refer to `../../docs/PERFORMANCE_ANALYSIS_PLAN.md` for the complete analysis methodology and objectives.
