import pandas as pd

df = pd.read_csv("com_calculator.csv")  # Load CSV

# Ensure numeric types
df['start_ms'] = pd.to_numeric(df['start_ms'][1:])
df['end_ms'] = pd.to_numeric(df['end_ms'][1:])
df['duration_ms'] = pd.to_numeric(df['duration_ms'][1:])

# Execution time stats
duration_mean = df['duration_ms'][1:].mean()
duration_max = df['duration_ms'][1:].max()
duration_min = df['duration_ms'][1:].min()
duration_jitter = df['duration_ms'][1:].std()

# Firing intervals: start[i] - end[i-1]
df['firing_interval_ms'] = df['start_ms'][1:] - df['end_ms'][1:].shift()

firing_mean = df['firing_interval_ms'][1:].mean()
firing_max = df['firing_interval_ms'][1:].max()
firing_min = df['firing_interval_ms'][1:].min()
firing_jitter = df['firing_interval_ms'][1:].std()

print("=== Execution Time (duration_ms) ===")
print(f"Mean:   {duration_mean:.3f} ms")
print(f"Max:    {duration_max:.3f} ms")
print(f"Min:    {duration_min:.3f} ms")
print(f"Jitter: {duration_jitter:.3f} ms (std dev)")

print("\n=== Firing Interval (start[i] - end[i-1]) ===")
print(f"Mean:   {firing_mean:.3f} ms")
print(f"Max:    {firing_max:.3f} ms")
print(f"Min:    {firing_min:.3f} ms")
print(f"Jitter: {firing_jitter:.3f} ms (std dev)")
