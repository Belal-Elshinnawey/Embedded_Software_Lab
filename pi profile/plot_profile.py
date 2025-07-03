import pandas as pd
import matplotlib.pyplot as plt

# ----- Config -----
plot_title = "Execution Time and Firing Interval Analysis (CoM Calculator loop)"

# ----- Load Data -----
df = pd.read_csv("com_calculator.csv")  # replace with your CSV file path

# Ensure numeric columns
df['start_ms'] = pd.to_numeric(df['start_ms'][1:])
df['end_ms'] = pd.to_numeric(df['end_ms'][1:])
df['duration_ms'] = pd.to_numeric(df['duration_ms'][1:])

# Calculate firing intervals: start[i] - end[i-1]
df['firing_interval_ms'] = df['start_ms'] - df['end_ms'].shift()

# Plot execution time
plt.figure(figsize=(10, 12))
plt.subplot(2, 1, 1)
plt.plot(df['index'], df['duration_ms'], marker='o', linestyle='-', color='blue')
plt.title(f"{plot_title} - Execution Time")
plt.ylabel("Duration (ms)")
plt.xticks([]) 
plt.grid(True)

# Plot firing intervals
plt.subplot(2, 1, 2)
plt.plot(df['index'], df['firing_interval_ms'], marker='x', linestyle='-', color='red')
plt.title(f"Firing Interval (start[i] - end[i-1])")
plt.xlabel("Index")
plt.ylabel("Interval (ms)")
plt.grid(True)

plt.tight_layout()
plt.show()
