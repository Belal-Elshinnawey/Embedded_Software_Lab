import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Load the combined CSV
df = pd.read_csv("combined_variant_scores.csv")

# ---------- NORMALIZE SCORES ----------
def normalize(col):
    return (col - col.min()) / (col.max() - col.min())

df['Dev_Norm'] = normalize(df['Development Score'])
df['Feas_Norm'] = normalize(df['Feasibility Score'])
df['Risk_Norm'] = normalize(df['Total Risk'])

# ---------- PARETO FILTERING ON NORMALIZED SCORES ----------
score_data = df[['Dev_Norm', 'Feas_Norm', 'Risk_Norm']].to_numpy()

def is_pareto_efficient(data):
    n_points = data.shape[0]
    is_efficient = np.ones(n_points, dtype=bool)
    for i in range(n_points):
        if is_efficient[i]:
            is_efficient[is_efficient] = np.any(data[is_efficient] > data[i], axis=1)
            is_efficient[i] = True
    return is_efficient

pareto_mask = is_pareto_efficient(score_data)
pareto_df = df[pareto_mask]

# ---------- PRINT PARETO VARIANTS ----------
print("=== Pareto-Optimal Variants ===")
print(pareto_df[['Index', 'Development Score', 'Feasibility Score', 'Total Risk']].to_string(index=False))

# ---------- 3D PLOTS ----------
fig = plt.figure(figsize=(14, 6))

# Subplot 1: All Variants (normalized)
ax1 = fig.add_subplot(121, projection='3d')
platforms = df['Platform'].astype('category').cat.codes  # convert text to numbers
ax1.scatter(
    df['Dev_Norm'], df['Feas_Norm'], df['Risk_Norm'],
    c=platforms, cmap='tab10', s=40
)
ax1.view_init(elev=20, azim=135)
ax1.set_title("All Variants (Normalized)")
ax1.set_xlabel("Normalized Dev Score")
ax1.set_ylabel("Normalized Feasibility")
ax1.set_zlabel("Normalized Risk")

# Subplot 2: Pareto Points (normalized)
ax2 = fig.add_subplot(122, projection='3d')
ax2.scatter(
    pareto_df['Dev_Norm'],
    pareto_df['Feas_Norm'],
    pareto_df['Risk_Norm'],
    c='red', s=80, edgecolors='black'
)
ax2.view_init(elev=20, azim=135)
ax2.set_title("Pareto-Optimal Variants (Normalized)")
ax2.set_xlabel("Normalized Dev Score")
ax2.set_ylabel("Normalized Feasibility")
ax2.set_zlabel("Normalized Risk")

# Force axis to span full normalized space
ax2.set_xlim(0, 1)
ax2.set_ylim(0, 1)
ax2.set_zlim(0, 1)
plt.tight_layout()
plt.show()
# ---------- Define platform masks and colors ----------
rpi_mask = df['Platform'].str.contains('RPi', case=False)
de10_mask = df['Platform'].str.contains('DE10', case=False)

# Color mapping
colors = {
    'RPi': 'blue',
    'DE10': 'red'
}

# ---------- 2D PLOT: Development vs Risk ----------
fig1 = plt.figure()
plt.scatter(df.loc[rpi_mask, 'Dev_Norm'], df.loc[rpi_mask, 'Risk_Norm'], c=colors['RPi'], label='RPi', s=40)
plt.scatter(df.loc[de10_mask, 'Dev_Norm'], df.loc[de10_mask, 'Risk_Norm'], c=colors['DE10'], label='DE10', s=40)
plt.xlabel("Normalized Development Score")
plt.ylabel("Normalized Risk Score")
plt.title("Development vs Risk")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# ---------- 2D PLOT: Development vs Feasibility ----------
fig2 = plt.figure()
plt.scatter(df.loc[rpi_mask, 'Dev_Norm'], df.loc[rpi_mask, 'Feas_Norm'], c=colors['RPi'], label='RPi', s=40)
plt.scatter(df.loc[de10_mask, 'Dev_Norm'], df.loc[de10_mask, 'Feas_Norm'], c=colors['DE10'], label='DE10', s=40)
plt.xlabel("Normalized Development Score")
plt.ylabel("Normalized Feasibility Score")
plt.title("Development vs Feasibility")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# ---------- 2D PLOT: Risk vs Feasibility ----------
fig3 = plt.figure()
plt.scatter(df.loc[rpi_mask, 'Risk_Norm'], df.loc[rpi_mask, 'Feas_Norm'], c=colors['RPi'], label='RPi', s=40)
plt.scatter(df.loc[de10_mask, 'Risk_Norm'], df.loc[de10_mask, 'Feas_Norm'], c=colors['DE10'], label='DE10', s=40)
plt.xlabel("Normalized Risk Score")
plt.ylabel("Normalized Feasibility Score")
plt.title("Risk vs Feasibility")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
