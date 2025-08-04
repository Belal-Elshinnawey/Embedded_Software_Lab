import pandas as pd

# Load your scored variants
df = pd.read_csv("scored_variants.csv")

# Find the highest development score
max_score = df["Development Score"].max()

# Filter all variants tied at the top
top_variants = df[df["Development Score"] == max_score]

# Display
print(f"Best Variants Based on Development Score = {max_score}:")
print(top_variants.to_string(index=False))
