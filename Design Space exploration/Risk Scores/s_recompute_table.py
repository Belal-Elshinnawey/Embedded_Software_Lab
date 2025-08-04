import pandas as pd

# --- Step 1: Read the CSV ---
input_csv = "Control_Loop_Risk_Scores.csv"
df = pd.read_csv(input_csv)
score_name = "Total Risk"
# --- Step 2: Input score mapping ---
#PWM scores
# score_input = """
# Edge PWM 4, Center PWM 5
# DE10-Avalon 6, RPi-SPI 5
# CPU 12, FPGA 5
# """

# #Q-Decoder scores
# score_input = """
# LUT Decoder 4, Edge Decoder 6
# DE10-Avalon 6, RPi-SPI 5
# CPU 12, FPGA 5
# """

# #Image processing Scores
# score_input = """
# HSV 7, YUV 6
# DE10-Avalon 5, RPi-SPI 4
# CPU 4, FPGA 12
# """

# #Control loop Scores
score_input = """
DE10-Avalon 5, RPi-SPI 4
CPU 4, FPGA 12
"""

# --- Step 3: Parse input into list of value-to-score mappings ---
def parse_score_input(score_text):
    score_lines = score_text.strip().splitlines()
    score_dicts = []

    for line in score_lines:
        current_dict = {}
        pairs = line.split(',')
        for pair in pairs:
            key, value = pair.strip().rsplit(' ', 1)
            current_dict[key.strip()] = float(value)
        score_dicts.append(current_dict)

    return score_dicts

# Get list of mappings, assumed to match column order
score_mappings = parse_score_input(score_input)

# --- Step 4: Dynamically match columns to value sets ---
column_score_map = dict(zip(df.columns[:-1], score_mappings))  # Skip the last column (original score)

# --- Step 5: Recompute score using those maps ---
def calculate_score(row):
    score = 1
    for column, mapping in column_score_map.items():
        value = row[column]
        score += mapping.get(value, 1)  # Default weight is 1
    return (1/score)*100

df[score_name] = df.apply(calculate_score, axis=1)

# --- Step 6: Output ---
df.to_csv(input_csv, index=False)
print(df)
