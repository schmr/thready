import pandas as pd

data = pd.read_csv("thready-performance-benchmark.csv")
perf = data["Stdout"] / data["JobRuntime"]
assert perf.sum() / len(perf) > 9.6e6
