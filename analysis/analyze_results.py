#!/usr/bin/env python3
"""Decode and summarise the simulator's result CSVs.

Every CSV in ``results/`` is one experiment: the Logger appends one line per
simulation run, so each file holds a 10-point sweep of the class-2 offered load
(1, 2, ... 10 Mbps) at a fixed transmission rate R = 10 Mbps.

Columns (written by ``Sink::Stop`` in ``src/Sink.h``):

    TS1_DELAY        mean end-to-end delay of traffic source 1 [s]
    TS2_DELAY        mean end-to-end delay of traffic source 2 [s]
    TS1_THROUGHPUT   bits of TS1 delivered to the sink / SimTime [bps]
    TS2_THROUGHPUT   bits of TS2 delivered to the sink / SimTime [bps]

TS1/TS2 are keyed on ``packet.source_id``, NOT on priority, so the same column
always refers to the same traffic source regardless of the priority policy.
Loss is derived as ``1 - throughput / offered load``.

Uses the standard library only -- no pandas required.
"""

import csv
import os

HERE = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(HERE, os.pardir, "results")

# Inferred simulation parameters -- see README "How the file names were decoded".
R_BPS = 10e6            # transmission rate of the shared link
TS1_LOAD_BPS = 100e3    # class-1 offered load, constant across the sweep
TS2_LOADS_BPS = [n * 1e6 for n in range(1, 11)]  # class-2 sweep, one row each

# filename -> (batch size, priority policy, human label)
EXPERIMENTS = [
    ("ex1.csv",     1,  "none", "Single arrivals, no priority"),
    ("ex2.csv",     1,  "TS1",  "Single arrivals, priority to TS1"),
    ("ex22.csv",    1,  "TS2",  "Single arrivals, priority to TS2"),
    ("ex34np.csv",  4,  "none", "Batch B=4, no priority"),
    ("ex34p1.csv",  4,  "TS1",  "Batch B=4, priority to TS1"),
    ("ex34p2.csv",  4,  "TS2",  "Batch B=4, priority to TS2"),
    ("ex38np.csv",  8,  "none", "Batch B=8, no priority"),
    ("ex38p1.csv",  8,  "TS1",  "Batch B=8, priority to TS1"),
    ("ex38p2.csv",  8,  "TS2",  "Batch B=8, priority to TS2"),
    ("ex316np.csv", 16, "none", "Batch B=16, no priority"),
    ("ex316p1.csv", 16, "TS1",  "Batch B=16, priority to TS1"),
    ("ex316p2.csv", 16, "TS2",  "Batch B=16, priority to TS2"),
]


def load(filename):
    """Return the CSV as a list of dicts of floats."""
    path = os.path.join(RESULTS_DIR, filename)
    with open(path, newline="") as handle:
        rows = [{k: float(v) for k, v in row.items()}
                for row in csv.DictReader(handle)]
    return rows


def annotate(rows):
    """Attach offered load and derived loss to each row of a sweep."""
    out = []
    for i, row in enumerate(rows):
        ts2_offered = TS2_LOADS_BPS[i] if i < len(TS2_LOADS_BPS) else float("nan")
        total_offered = TS1_LOAD_BPS + ts2_offered
        out.append({
            "ts2_offered": ts2_offered,
            "rho": total_offered / R_BPS,
            "ts1_delay": row["TS1_DELAY"],
            "ts2_delay": row["TS2_DELAY"],
            "ts1_thr": row["TS1_THROUGHPUT"],
            "ts2_thr": row["TS2_THROUGHPUT"],
            "ts1_loss": 1.0 - row["TS1_THROUGHPUT"] / TS1_LOAD_BPS,
            "ts2_loss": 1.0 - row["TS2_THROUGHPUT"] / ts2_offered,
            "total_thr": row["TS1_THROUGHPUT"] + row["TS2_THROUGHPUT"],
        })
    return out


def per_experiment_table():
    """One line per experiment: behaviour at light load and at saturation."""
    header = ("{:<12} {:>5} {:>6} | {:>10} {:>10} | {:>10} {:>10} | "
              "{:>8} {:>8} | {:>10}").format(
        "file", "batch", "prio",
        "D1 lo (ms)", "D1 hi (ms)", "D2 lo (ms)", "D2 hi (ms)",
        "L1 hi", "L2 hi", "Thr hi Mbps")
    print(header)
    print("-" * len(header))
    for filename, batch, prio, _label in EXPERIMENTS:
        rows = annotate(load(filename))
        lo, hi = rows[0], rows[-1]
        print("{:<12} {:>5} {:>6} | {:>10.3f} {:>10.3f} | {:>10.3f} {:>10.3f} | "
              "{:>7.1f}% {:>7.1f}% | {:>10.3f}".format(
                  filename, batch, prio,
                  lo["ts1_delay"] * 1e3, hi["ts1_delay"] * 1e3,
                  lo["ts2_delay"] * 1e3, hi["ts2_delay"] * 1e3,
                  hi["ts1_loss"] * 100, hi["ts2_loss"] * 100,
                  hi["total_thr"] / 1e6))


def priority_effect_table():
    """For each batch size, compare the three priority policies at saturation."""
    groups = {}
    for filename, batch, prio, _label in EXPERIMENTS:
        groups.setdefault(batch, {})[prio] = annotate(load(filename))[-1]

    header = ("{:<6} {:<6} | {:>12} {:>12} | {:>9} {:>9} | {:>12}").format(
        "batch", "prio", "D1 (ms)", "D2 (ms)", "loss1", "loss2", "total Mbps")
    print(header)
    print("-" * len(header))
    for batch in sorted(groups):
        for prio in ("none", "TS1", "TS2"):
            row = groups[batch].get(prio)
            if row is None:
                continue
            print("{:<6} {:<6} | {:>12.3f} {:>12.3f} | {:>8.1f}% {:>8.1f}% | "
                  "{:>12.3f}".format(
                      batch, prio,
                      row["ts1_delay"] * 1e3, row["ts2_delay"] * 1e3,
                      row["ts1_loss"] * 100, row["ts2_loss"] * 100,
                      row["total_thr"] / 1e6))
        print()


def full_sweep(filename):
    """Print the whole 10-point load sweep for one experiment."""
    print("Load sweep for {}".format(filename))
    header = ("{:>10} {:>6} | {:>10} {:>10} | {:>12} {:>12} | "
              "{:>8} {:>8}").format(
        "B2 (Mbps)", "rho", "D1 (ms)", "D2 (ms)",
        "Thr1 (kbps)", "Thr2 (Mbps)", "loss1", "loss2")
    print(header)
    print("-" * len(header))
    for row in annotate(load(filename)):
        print("{:>10.1f} {:>6.2f} | {:>10.3f} {:>10.3f} | {:>12.1f} {:>12.3f} | "
              "{:>7.1f}% {:>7.1f}%".format(
                  row["ts2_offered"] / 1e6, row["rho"],
                  row["ts1_delay"] * 1e3, row["ts2_delay"] * 1e3,
                  row["ts1_thr"] / 1e3, row["ts2_thr"] / 1e6,
                  row["ts1_loss"] * 100, row["ts2_loss"] * 100))
    print()


def batch_effect_table():
    """Isolate the effect of the batch size with the priority policy held fixed."""
    header = "{:<6} | {:>12} {:>12} | {:>9} {:>9}".format(
        "batch", "D2 lo (ms)", "D2 hi (ms)", "loss2 lo", "loss2 hi")
    print(header)
    print("-" * len(header))
    for filename, batch, prio, _label in EXPERIMENTS:
        if prio != "none":
            continue
        rows = annotate(load(filename))
        lo, hi = rows[0], rows[-1]
        print("{:<6} | {:>12.3f} {:>12.3f} | {:>8.1f}% {:>8.1f}%".format(
            batch, lo["ts2_delay"] * 1e3, hi["ts2_delay"] * 1e3,
            lo["ts2_loss"] * 100, hi["ts2_loss"] * 100))
    print()


def main():
    print("=" * 78)
    print("1. Per-experiment summary (lo = B2 1 Mbps, hi = B2 10 Mbps)")
    print("=" * 78)
    per_experiment_table()
    print()

    print("=" * 78)
    print("2. Effect of the priority policy, at saturation (B2 = 10 Mbps)")
    print("=" * 78)
    priority_effect_table()

    print("=" * 78)
    print("3. Effect of the batch size, no-priority policy only")
    print("=" * 78)
    batch_effect_table()

    print("=" * 78)
    print("4. Full load sweeps for the single-arrival experiments")
    print("=" * 78)
    for filename in ("ex1.csv", "ex2.csv", "ex22.csv"):
        full_sweep(filename)


if __name__ == "__main__":
    main()
