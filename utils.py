import os
import re

def extract_standard_metrics(node_count):
    """
    Unified extraction for ALL 5 Figures.
    Pulls PDR, Delay, Throughput from nresults_XX.txt
    Pulls Congestion and Optimal Probability from full_log_XX.txt
    """
    # --- Part 1: Summary Results (Figs 8, 9, 10) ---
    summary_file = f"nresults_{node_count}.txt"
    pdr, delay, throughput = 0.0, 0.0, 0.0
    
    if os.path.exists(summary_file):
        try:
            with open(summary_file, "r", encoding="utf-8") as f:
                data = f.read()
                # Extract PDR [cite: 72]
                pdr_match = re.search(r"Ratio\): ([\d.]+)", data)
                pdr = float(pdr_match.group(1)) if pdr_match else 0.0
                
                # Extract Delay (ms) [cite: 72, 213]
                delay_match = re.search(r"Delay : ([\d.]+)", data)
                delay = float(delay_match.group(1)) if delay_match else 0.0
                
                # Extract Throughput (Mbps) [cite: 72, 103]
                tp_match = re.search(r"Throughput: ([\d.]+)", data)
                throughput = float(tp_match.group(1)) if tp_match else 0.0
        except Exception as e:
            print(f"Error reading {summary_file}: {e}")

    # --- Part 2: Real-time Logs (Figs 7, 11) ---
    log_file = f"full_log_{node_count}.txt"
    cong_vals = []
    opt_prob = 0.0
    
    if os.path.exists(log_file):
        try:
            with open(log_file, "r") as f:
                for line in f:
                    # Capture Dynamic Congestion Rate (Fig 7) [cite: 223, 230]
                    if "CONGESTION_DATA: Rate=" in line:
                        cong_vals.append(float(line.split("=")[-1]))
                    # Capture Algorithmic Path Success (Fig 11) [cite: 297, 313]
                    if "OPTIMAL_RESULT: " in line:
                        opt_prob = float(line.split(": ")[-1])
        except Exception as e:
            print(f"Error reading {log_file}: {e}")

    # Average the congestion rate found during the 400s simulation [cite: 213, 215]
    avg_congestion = sum(cong_vals) / len(cong_vals) if cong_vals else 0.0
    
    # Return all 5 parameters needed for your graphs
    return pdr, delay, throughput, avg_congestion, opt_prob
