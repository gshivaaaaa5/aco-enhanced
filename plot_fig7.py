import matplotlib.pyplot as plt
import os

nodes = [10, 50, 100, 150, 200]
congestion_rates = []

# Read each file and find the maximum congestion experienced
for n in nodes:
    filename = f"results_{n}.txt"
    max_congestion = 0.0
    
    if os.path.exists(filename):
        with open(filename, 'r') as file:
            for line in file:
                if "METRIC_CONGESTION:" in line:
                    val = float(line.split(":")[1].strip())
                    if val > max_congestion:
                        max_congestion = val
        congestion_rates.append(max_congestion)
    else:
        congestion_rates.append(0.0)

# Draw the graph just like Figure 7
plt.figure(figsize=(8, 6))
plt.plot(nodes, congestion_rates, marker='s', color='black', linestyle='-')

plt.xlabel("Number of Vehicle", fontsize=12, fontweight='bold')
plt.ylabel("Congestion Rate", fontsize=12, fontweight='bold')
plt.title("Fig. 7. Congestion Rate Vs No. of Vehicle", fontsize=14, y=-0.15)

plt.xlim(0, 210)
plt.ylim(0, 1.1)
plt.xticks([0, 50, 100, 150, 200])

plt.tight_layout()
plt.savefig("Fig_7_Congestion.png", dpi=300)
print("Success! Graph saved as Fig_7_Congestion.png")
