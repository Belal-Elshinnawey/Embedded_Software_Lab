import pandas as pd

# Load score CSVs
pwm_df = pd.read_csv("PWM_Dev_Debug_Scores.csv")
decoder_df = pd.read_csv("Q-Decoder_Dev_Debug_Scores.csv")
filter_df = pd.read_csv("Image_Filter_Dev_Debug_Scores.csv")
control_df = pd.read_csv("Control_Loop_Dev_Debug_Scores.csv")

# Load your variant input file
variants = pd.read_csv("../Design_Variants_Table.csv")

# Clean column headers
for df in [pwm_df, decoder_df, filter_df, control_df, variants]:
    df.columns = df.columns.str.strip()

# Compute development scores
scores = []
for _, row in variants.iterrows():
    platform = row["Platform"]

    pwm_score = pwm_df.query(
        'Platform == @platform and Location == @row["PWM Location"] and `PWM Type` == @row["PWM Type"]'
    )["Dev & Debug Score"].values[0]

    decoder_score = decoder_df.query(
        'Platform == @platform and Location == @row["Encoder Location"] and `Decoder Type` == @row["Decoder Type"]'
    )["Dev & Debug Score"].values[0]

    filter_score = filter_df.query(
        'Platform == @platform and Location == @row["Image Processing Location"] and `Filter Type` == @row["Filter Type"]'
    )["Dev & Debug Score"].values[0]

    control_score = control_df.query(
        'Platform == @platform and Location == @row["Control Loop Location"]'
    )["Dev & Debug Score"].values[0]

    total = pwm_score + decoder_score + filter_score + control_score
    scores.append(total)

# Output result
variants["Development Score"] = scores
variants.to_csv("scored_variants.csv", index=False)
print("Saved scored variants to 'scored_variants.csv'")
