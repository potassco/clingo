import argparse

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pandas_ods_reader import read_ods

# select the measure to plot

argparser = argparse.ArgumentParser("plot measures for systems")
argparser.add_argument(
    "measure", type=str, default="time,timeouts", help="the measure to plot"
)
argparser.add_argument(
    "file", type=str, default="results/result.ods", help="the ODS file to read from"
)

args = argparser.parse_args()

measure_names = args.measure.split(",")
if len(measure_names) == 0 or len(measure_names) > 2:
    argparser.error("please provide one or two measures to plot")

# NOTE: the file has to be opened and saved once with LibreOffice because the
# generated ODS file does not contain computed values.
df = read_ods(args.file, sheet=1, headers=False)

stride = 1
system_row = df.iloc[0]
measures_row = df.iloc[1]
while pd.isna(system_row.iloc[1 + stride]) or system_row.iloc[1 + stride] == "":
    stride += 1


sum_index = df.index[df.iloc[:, 0] == "SUM"].tolist()[-1]

values_to_plot = []
for measure_name in measure_names:
    i = 1
    for measure in measures_row[1 : stride + 1]:
        if measure == measure_name:
            values_to_plot.append(df.iloc[sum_index, i::stride].astype(float).tolist())
            break
        i += 1
    else:
        argparser.error(f"measure '{measure_name}' not found in the file")

systems = df.iloc[0, 1::stride][:-3].tolist()

combined = sorted(list(zip(systems, *values_to_plot)), key=lambda x: x[1:])
systems, *values_to_plot = zip(*combined)
systems = list(systems)
values_to_plot = [list(vals) for vals in values_to_plot]

print("Systems:", systems)
for name, values in zip(measure_names, values_to_plot):
    print(f"{name}:", values)

x = np.arange(len(systems))
width = 0.35

fig, ax1 = plt.subplots(figsize=(12, 6))

bars1 = ax1.bar(
    x - width / 2, values_to_plot[0], width, label=measure_names[0], color="blue"
)
ax1.set_ylabel(measure_names[0].capitalize())
ax1.set_xticks(x)
ax1.set_xticklabels(systems, rotation=45, ha="right")
ax1.tick_params(axis="y", labelcolor="blue")
ax1.set_ylim(bottom=min(values_to_plot[0]))

if len(measure_names) > 1:
    ax2 = ax1.twinx()
    bars2 = ax2.bar(
        x + width / 2,
        values_to_plot[1],
        width,
        label=measure_names[1].capitalize(),
        color="orange",
    )
    ax2.set_ylabel(measure_names[1].capitalize())
    ax2.tick_params(axis="y", labelcolor="orange")
    ax2.set_ylim(bottom=min(values_to_plot[1]))

if len(measure_names) > 1:
    plt.title(
        f"System Benchmark: {measure_names[0].capitalize()} and {measure_names[1].capitalize()} (Sorted)"
    )
else:
    plt.title(f"System Benchmark: {measure_names[0].capitalize()} (Sorted)")
fig.tight_layout()
plt.show()
