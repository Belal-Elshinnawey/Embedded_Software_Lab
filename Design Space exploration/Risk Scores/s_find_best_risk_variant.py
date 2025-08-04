import pandas as pd

# Load your scored variants
df = pd.read_csv("scored_variants.csv")

# Find the highest development score
max_score = df["Total Risk"].max()

# Filter all variants tied at the top
top_variants = df[df["Total Risk"] == max_score]

# Display
print(f"Best Variants Based on Total Risk Score = {max_score}:")
print(top_variants.to_string(index=False))
