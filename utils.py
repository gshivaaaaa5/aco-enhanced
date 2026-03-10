import os
import re

# ... (Keep all your existing get_list_from_file, read_config_file, etc. here) ...

def extract_standard_metrics(node_count):
    """
    Standardizes data extraction for the EACO-DE project.
    Reads 'nresults_XX.txt' and returns PDR, Delay, and Throughput.
    """
    filename = f"nresults_{node_count}.txt"
    
    if not os.path.exists(filename):
        print(f"Error: {filename} not found!")
        return 0.0, 0.0, 0.0

    try:
        with open(filename, "r", encoding="utf-8") as f:
            data = f.read()
            
            # 1. Extract PDR (%) - Matches the "FINAL PDR (Packet Delivery Ratio):" line
            pdr_match = re.search(r"Ratio\): ([\d.]+)", data)
            pdr = float(pdr_match.group(1)) if pdr_match else 0.0
            
            # 2. Extract Delay (ms) - Matches the "> Avg Delay :" line
            delay_match = re.search(r"Delay : ([\d.]+)", data)
            delay = float(delay_match.group(1)) if delay_match else 0.0
            
            # 3. Extract Throughput (Mbps) - Matches the "> Throughput:" line
            tp_match = re.search(r"Throughput: ([\d.]+)", data)
            throughput = float(tp_match.group(1)) if tp_match else 0.0
            
            return pdr, delay, throughput
            
    except Exception as e:
        print(f"Error processing {filename}: {e}")
        return 0.0, 0.0, 0.0
