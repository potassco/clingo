import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pandas_ods_reader import read_ods

# NOTE: the file has to be opened and saved once with LibreOffice because the
# generated ODS file does not contain computed values.
df = read_ods("results/results.ods", sheet=1, headers=False)

stride = 1
first_row = df.iloc[0]
while pd.isna(first_row[1 + stride]) or first_row[1 + stride] == "":
    stride += 1

sum_index = df.index[df.iloc[:, 0] == "SUM"].tolist()[-1] + 1

sum_row = df.iloc[sum_index]
systems = df.iloc[0, 1::stride][:-3].tolist()
times = df.iloc[sum_index, 1::stride][:-3].astype(float).tolist()
timeouts = df.iloc[sum_index, 2::stride][:-3].astype(float).tolist()

assert (
    len(systems) == len(times) == len(timeouts)
), "Mismatched lengths between systems, times, and timeouts"

systems_sorted, times_sorted, timeouts_sorted = zip(
    *sorted(list(zip(systems, times, timeouts)), key=lambda x: (x[1], x[2]))
)

x = np.arange(len(systems_sorted))
width = 0.35

fig, ax1 = plt.subplots(figsize=(12, 6))

bars1 = ax1.bar(x - width / 2, times_sorted, width, label="Runtime", color="blue")
ax1.set_ylabel("Runtime")
ax1.set_xticks(x)
ax1.set_xticklabels(systems_sorted, rotation=45, ha="right")
ax1.tick_params(axis="y", labelcolor="blue")
ax1.set_ylim(bottom=min(times_sorted))

ax2 = ax1.twinx()
bars2 = ax2.bar(x + width / 2, timeouts_sorted, width, label="Timeouts", color="orange")
ax2.set_ylabel("Timeouts")
ax2.tick_params(axis="y", labelcolor="orange")
ax2.set_ylim(bottom=min(timeouts_sorted))

plt.title("System Benchmark: Runtime and Timeouts (Sorted)")
fig.tight_layout()
plt.show()
