import matplotlib.pyplot as plt

# Read the data from the text file
times = []
positions = []
with open('pi_simulated_encoder_data.txt', 'r') as file:
    for line in file:
        parts = line.strip().split('|')
        time_str = parts[0].split(':')[1].strip().split()[0]
        pos_str = parts[1].split(':')[1].strip()
        times.append(float(time_str))
        positions.append(int(pos_str))

# Normalize times to start from zero for plotting
start_time = times[0]
normalized_times = [t - start_time for t in times]

# Plot with x-axis spaced by normalized times
plt.figure(figsize=(10, 5))
plt.plot(normalized_times, positions, marker='o')

max_time = max(normalized_times)
tick_step = 0.5
ticks = [round(x * tick_step, 3) for x in range(int(max_time / tick_step) + 2)]
plt.xticks(ticks, rotation=45)

plt.xlabel('Time (seconds)')
plt.ylabel('Position')
plt.title('Position vs Time')
plt.grid(True)
plt.tight_layout()
plt.show()
