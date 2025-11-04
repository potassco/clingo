import matplotlib.pyplot as plt
import numpy as np
from pandas_ods_reader import read_ods

# NOTE: The ODS file has to be saved once with LibreOffice to compute values
df = read_ods("results/results.ods", sheet=1, headers=False)

stride = 1
first_row = df.iloc[0]
while pd.isna(first_row[1 + stride]) or first_row[1 + stride] == "":
    stride += 1

sum_index = df.index[df.iloc[:, 0] == "SUM"].tolist()[-1]

# Extract system names from first row, skipping last 3 entries
systems = df.iloc[0, 1::stride][:-3].tolist()

print(systems)

times = df.iloc[2 : sum_index - 2, 1::stride].iloc[:, :-3].astype(float)

plt.figure(figsize=(14, 8))

# Plot each system's sorted runtimes as a step plot
for i, system in enumerate(systems):
    times = times.iloc[:, i].dropna().sort_values().values
    x = np.arange(1, len(times) + 1)
    plt.step(x, times, where="post", label=system)

plt.xlabel("Number of instances solved")
plt.ylabel("Runtime (seconds)")
plt.title("Combined Cactus Plot for Systems")
plt.grid(True)
plt.legend(loc="best")
plt.tight_layout()
plt.show()
