import pandas as pd

# --- Step 1: Load the three files ---
dev_df = pd.read_csv("development scores/scored_variants.csv")
feas_df = pd.read_csv("feasability scores/scored_variants.csv")
risk_df = pd.read_csv("Risk Scores/scored_variants.csv")

# --- Step 2: Normalize column names ---
# Handle typo: 'latform' → 'Platform'
if 'latform' in feas_df.columns:
    feas_df.rename(columns={'latform': 'Platform'}, inplace=True)

# --- Step 3: Remove score columns temporarily to get join keys ---
# We'll join on all other columns except the score columns
score_cols = {
    'Development Score': 'Development Score',
    'Feasibility Score': 'Feasibility Score',
    'Total Risk': 'Total Risk'
}

# Remove score columns for merge keys
dev_features = dev_df.drop(columns=[score_cols['Development Score']])
feas_features = feas_df.drop(columns=[score_cols['Feasibility Score']])
risk_features = risk_df.drop(columns=[score_cols['Total Risk']])

# --- Step 4: Merge the dataframes on all feature columns ---
merged_df = dev_features.copy()
merged_df[score_cols['Development Score']] = dev_df[score_cols['Development Score']]
merged_df = pd.merge(merged_df, feas_df, on=list(dev_features.columns), how='inner')
merged_df = pd.merge(merged_df, risk_df, on=list(dev_features.columns), how='inner')

# --- Step 5: Add index column ---
merged_df.insert(0, "Index", range(1, len(merged_df) + 1))

# --- Step 6: Save the final result ---
merged_df.to_csv("combined_variant_scores.csv", index=False)

# Optional: Print preview
print(merged_df.head())
