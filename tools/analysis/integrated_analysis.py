"""
Integrated Performance Analysis Script

Runs all performance analyses and generates a comprehensive report:
- Transmission latency analysis
- Measurement accuracy analysis
- Trip response time analysis
- Multi-metric correlation analysis
"""

import subprocess
import sys
from pathlib import Path
from datetime import datetime

# Output directory
OUTPUT_DIR = 'results'
Path(OUTPUT_DIR).mkdir(parents=True, exist_ok=True)


def print_header(title):
    """Print formatted section header."""
    print("\n" + "="*70)
    print(f"  {title}")
    print("="*70 + "\n")


def run_analysis(script_name, description):
    """Run an analysis script and capture results."""
    print_header(description)
    
    try:
        result = subprocess.run(
            [sys.executable, script_name],
            capture_output=True,
            text=True,
            check=True
        )
        print(result.stdout)
        if result.stderr:
            print("Warnings/Errors:", result.stderr)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running {script_name}:")
        print(e.stdout)
        print(e.stderr)
        return False
    except FileNotFoundError:
        print(f"[ERROR] Script not found: {script_name}")
        return False


def main():
    """Run integrated performance analysis."""
    start_time = datetime.now()
    
    print_header("INTEGRATED PERFORMANCE ANALYSIS")
    print("Smart Energy Monitoring System")
    print(f"Analysis started: {start_time.strftime('%Y-%m-%d %H:%M:%S')}\n")
    
    # Track results
    results = {}
    
    # Run individual analyses
    analyses = [
        ("latency_analysis.py", "Transmission Latency Analysis"),
        ("accuracy_analysis.py", "Measurement Accuracy Analysis"),
        ("trip_response_analysis.py", "Trip Response Time Analysis"),
        ("system_metrics.py", "System Reliability & Correlation Analysis"),
    ]
    
    for script, description in analyses:
        success = run_analysis(script, description)
        results[description] = "[SUCCESS]" if success else "[FAILED]"
    
    # Summary
    end_time = datetime.now()
    duration = (end_time - start_time).total_seconds()
    
    print_header("ANALYSIS SUMMARY")
    print(f"Analysis completed: {end_time.strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Total duration: {duration:.2f} seconds\n")
    
    print("Results:")
    for analysis, status in results.items():
        print(f"  {status} {analysis}")
    
    print(f"\nAll results saved to: {OUTPUT_DIR}/")
    print("\nNext steps:")
    print("  1. Review individual analysis reports in results/ subdirectories")
    print("  2. Check generated figures for visualizations")
    print("  3. Run generate_report.py for comprehensive documentation")


if __name__ == "__main__":
    main()
