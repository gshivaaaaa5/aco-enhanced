import matplotlib.pyplot as plt
import os

nodes = [10, 50, 100, 150, 200]
pdr_rates = []

# Read each file and find the FINAL PDR percentage
for n in nodes:
    filename = f"results_{n}.txt"
    pdr = 0.0
    
    if os.path.exists(filename):
        with open(filename, 'r') as file:
            for line in file:
                if "FINAL PDR" in line:
                    # Extract the number from " FINAL PDR (Packet Delivery Ratio): 94.2 %"
                    parts = line.split(":")
                    if len(parts) > 1:
                        num_str = parts[1].replace("%", "").strip()
                        pdr = float(num_str)
        pdr_rates.append(pdr)
    else:
        pdr_rates.append(0.0)

# Draw the Reliability graph
plt.figure(figsize=(8, 6))
plt.plot(nodes, pdr_rates, marker='o', color='blue', linestyle='-', linewidth=2, markersize=8)

plt.xlabel("Number of Vehicle", fontsize=12, fontweight='bold')
plt.ylabel("Reliability (PDR %)", fontsize=12, fontweight='bold')
plt.title("Reliability Vs No. of Vehicle", fontsize=14, y=-0.15)

plt.xlim(0, 210)
plt.ylim(0, 105)
plt.xticks([0, 50, 100, 150, 200])

plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("Reliability_Graph.png", dpi=300)
print("Success! Graph saved as Reliability_Graph.png")
