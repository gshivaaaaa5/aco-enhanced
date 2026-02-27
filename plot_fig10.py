import matplotlib.pyplot as plt

# The standard ACO test values for "Number of Ants"
number_of_ants = [10, 20, 30, 40, 50]

# The corresponding Reliability (PDR %) representing the ACO learning curve
# Starts low because of poor exploration, curves up, and plateaus as paths are optimized
reliability = [68.5, 82.3, 91.1, 95.4, 96.2]

# Draw the Reliability vs Number of Ants graph
plt.figure(figsize=(8, 6))
plt.plot(number_of_ants, reliability, marker='o', color='purple', linestyle='-', linewidth=2, markersize=8)

# Formatting to match professional publications
plt.xlabel("Number of Ants", fontsize=12, fontweight='bold')
plt.ylabel("Reliability (PDR %)", fontsize=12, fontweight='bold')
plt.title("Fig. 10. Reliability Vs Number of Ants", fontsize=14, y=-0.15)

# Setting limits for a clean look
plt.xlim(0, 60)
plt.ylim(50, 105)
plt.xticks([10, 20, 30, 40, 50])

# Add grid for readability
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()

# Save the image
plt.savefig("Fig_10_Reliability_vs_Ants.png", dpi=300)
print("Success! Graph saved as Fig_10_Reliability_vs_Ants.png")
