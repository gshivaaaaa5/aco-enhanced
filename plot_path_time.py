import matplotlib.pyplot as plt
import os

nodes = [10, 50, 100, 150, 200]
path_times = []

# Read each file and find the average Path Discovery Time
for n in nodes:
    filename = f"results_{n}.txt"
    times = []
    
    if os.path.exists(filename):
        with open(filename, 'r') as file:
            for line in file:
                if "METRIC_PATH_TIME:" in line:
                    # Extract the time value (e.g., from "METRIC_PATH_TIME: 0.045 seconds")
                    parts = line.split(":")
                    if len(parts) > 1:
                        time_str = parts[1].replace("seconds", "").strip()
                        times.append(float(time_str))
        
        # Calculate the average time for this number of nodes (convert to milliseconds for a better looking graph)
        if len(times) > 0:
            avg_time_ms = (sum(times) / len(times)) * 1000 
            path_times.append(avg_time_ms)
        else:
            path_times.append(0.0)
    else:
        path_times.append(0.0)

# Draw the Path Discovery Time graph
plt.figure(figsize=(8, 6))
plt.plot(nodes, path_times, marker='^', color='red', linestyle='-', linewidth=2, markersize=8)

plt.xlabel("Number of Vehicle", fontsize=12, fontweight='bold')
plt.ylabel("Path Discovery Time (ms)", fontsize=12, fontweight='bold')
plt.title("Path Discovery Time Vs No. of Vehicle", fontsize=14, y=-0.15)

plt.xlim(0, 210)
# Adjust Y-axis to fit the data nicely
plt.ylim(0, max(path_times) + 100 if max(path_times) > 0 else 1000) 
plt.xticks([0, 50, 100, 150, 200])

plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("Path_Discovery_Time.png", dpi=300)
print("Success! Graph saved as Path_Discovery_Time.png")
