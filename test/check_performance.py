import pandas as pd

EVENTS_PER_SEC_OPT = 9.6e6
EVENTS_PER_SEC = 8.36e6

data = pd.read_csv("thready-performance-benchmark.csv")
perf = data["Stdout"] / data["JobRuntime"]
current = perf.sum() / len(perf) + perf.std()
print("%f events/second" % current)
assert current > EVENTS_PER_SEC_OPT
