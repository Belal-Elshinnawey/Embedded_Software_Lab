import pandas as pd

# Load your scored variants
df = pd.read_csv("scored_variants.csv")

# Find the highest Feasability Score
max_score = df["Feasability Score"].max()

# Filter all variants tied at the top
top_variants = df[df["Feasability Score"] == max_score]

# Display
print(f"Best Variants Based on Feasability Score = {max_score}:")
print(top_variants.to_string(index=False))
