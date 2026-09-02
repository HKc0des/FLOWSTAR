#!/usr/bin/env python3
import csv
import glob
import os
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

files = glob.glob(os.path.join(os.path.dirname(__file__), "raw", "seed-*", "summary.csv"))
values = defaultdict(lambda: [0.0, 0.0, 0])
for path in files:
    with open(path, newline="") as fh:
        for row in csv.DictReader(fh):
            key = (int(row["mode"]), int(row["workload"]))
            values[key][0] += float(row["rx"])
            values[key][1] += float(row["max_queue"])
            values[key][2] += 1

names = {0: "NoCtrl", 1: "BaseCC", 2: "CBFC", 3: "FlowStar", 4: "FS+I1", 5: "FS+I2"}
colors = {1: "#4472C4", 2: "#ED7D31", 3: "#70AD47", 4: "#A5A5A5", 5: "#FFC000"}
os.makedirs(os.path.dirname(__file__), exist_ok=True)

def plot_metric(field, ylabel, filename, title):
    workloads = sorted({w for m, w in values})
    modes = sorted({m for m, w in values})
    fig, axes = plt.subplots(1, len(workloads), figsize=(16, 4.5), sharey=False)
    if len(workloads) == 1:
        axes = [axes]
    for ax, workload in zip(axes, workloads):
        xs, ys, labels = [], [], []
        for mode in modes:
            key = (mode, workload)
            if key not in values:
                continue
            total = values[key][0 if field == "rx" else 1]
            count = values[key][2]
            xs.append(len(labels)); ys.append(total / count); labels.append(names.get(mode, f"Mode {mode}"))
        ax.bar(xs, ys, color=[colors.get(w, "#5B9BD5") for w in [workload] * len(xs)])
        ax.set_title(f"W{workload}")
        ax.set_xticks(xs, labels, rotation=45, ha="right")
        ax.grid(axis="y", alpha=0.3)
        ax.set_axisbelow(True)
        for x, y in zip(xs, ys):
            if y > 0:
                ax.text(x, y, f"{y:.3g}", ha="center", va="bottom", fontsize=8, rotation=90)
    fig.suptitle(title)
    fig.supxlabel("Configuration mode")
    fig.supylabel(ylabel)
    fig.tight_layout()
    fig.savefig(os.path.join(os.path.dirname(__file__), filename), dpi=180, bbox_inches="tight")
    plt.close(fig)

plot_metric("rx", "Mean packets received", "throughput-matplotlib.png", "FlowStar Throughput (existing CSV values)")
plot_metric("queue", "Mean maximum queue (packets)", "queue-occupancy-matplotlib.png", "FlowStar Queue Occupancy (existing CSV values)")
print("Wrote throughput-matplotlib.png and queue-occupancy-matplotlib.png")
