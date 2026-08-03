# Priority Queueing Simulator (COST / C++)

A discrete-event simulation of a **two-class priority queueing system** sharing a
single transmission link, built on the [COST](http://www.ita.cs.rpi.edu/) component
simulation framework. Two traffic sources feed one queue module with two separate
FIFO buffers (high priority and low priority) draining into a common server of rate
*R*; a sink measures per-source delay and throughput, and a logger writes traces and
result rows. The point of the study is to measure what a **strict non-preemptive
priority discipline** actually buys you: how much delay one class can save, what the
other class pays for it, and how that trade-off changes when the arrivals become
bursty (batch arrivals) instead of one-at-a-time.

Second-year Network Engineering coursework (UPF, 2023).

---

## Why this approach

The model is deliberately minimal so the priority effect is visible in isolation:

- **One shared server, two buffers.** `QueueModule` keeps a `queue_HP` and a
  `queue_LP` (`src/QueueModule.h`). A packet is filed by its `priority` field.
- **Non-preemptive, strict priority.** When the service timer fires
  (`QueueModule::endService`) the module always looks at `queue_HP` first and only
  falls back to `queue_LP` when the high-priority buffer is empty. A packet already
  in service is never interrupted — that is what makes the discipline
  non-preemptive, and it is why the protected class still sees a residual delay of
  roughly one low-priority transmission time.
- **Finite buffers.** Each buffer holds at most `Q_HP` / `Q_LP` packets; anything
  arriving at a full buffer is counted as blocked and dropped. This is what turns
  burstiness into measurable loss rather than just unbounded delay.
- **Two source models.** `TrafficSource` emits one packet per timer expiry;
  `TrafficSourceBatch` emits `BATCHSIZE` packets at the *same* simulated instant.
  Both support Markovian (exponential) or deterministic inter-arrival times and
  packet sizes. Swapping one include in `QSim.cc` switches between them.
- **Classification is decoupled from measurement.** The queue sorts on
  `packet.priority`; the sink accumulates on `packet.source_id`. So the result
  columns always refer to the same *traffic source* whatever the priority policy is,
  which is exactly what makes the three policies comparable row by row.

---

## Structure

```
src/
  QSim.cc               top-level component: wires sources -> queue -> sink, plus logging;
                        main() parses the 15 simulation parameters from argv
  QueueModule.h         the two-buffer, strict non-preemptive priority server
  TrafficSourceBatch.h  batch traffic source (BATCHSIZE packets per arrival epoch)
  TrafficSource.h       single-arrival traffic source
  Sink.h                per-source delay / throughput accumulation; writes the result row
  FIFO.h                thin deque wrapper used for both buffers
  Tools.h               the Packet struct
  Logger.h              writes traces.txt and appends rows to results.csv
  build_local           the two-step build script (see below)
  COST/                 THIRD-PARTY simulation framework - see src/COST/README.md
results/
  ex*.csv               13 recorded result files, one experiment each (decoded below)
  traces.txt            event trace of the final run (first 1000 events)
analysis/
  analyze_results.py    decodes the CSVs and prints the results tables below (stdlib only)
```

---

## How to build and run

**Not executed here.** This repository was assembled on Windows; the COST
translator `src/COST/cxx` is a 32-bit Linux ELF binary and the project was never
compiled during packaging. The commands below are copied verbatim from the original
`build_local` script and are the ones the project was actually built with on the
lab machines.

COST is not plain C++: `QSim.cc` uses `component` / `inport` / `outport` /
`connect` keywords which a preprocessor (`cxx`) translates into ordinary C++ before
`g++` sees them.

```bash
cd src
./COST/cxx QSim.cc          # generates QSim.cxx and compcxx_QSim.h
g++ -Wall -o QSim QSim.cxx  # compile the generated C++
```

`QSim.cxx` and `compcxx_QSim.h` are **generated**, not source, and are gitignored.

Running the simulator takes 15 positional arguments (see `main()` in
`src/QSim.cc`):

```
./QSim R Q_1 Q_2  B_1 EL_1 type_gen_1 type_size_1 source_1 priority_1 \
                  B_2 EL_2 type_gen_2 type_size_2 source_2 priority_2
```

| argument | meaning |
| --- | --- |
| `R` | transmission rate of the shared link [bps] |
| `Q_1`, `Q_2` | buffer size of the high- and low-priority queue [packets] |
| `B_k` | offered load of source *k* [bps] |
| `EL_k` | mean packet size of source *k* [bits] |
| `type_gen_k` | inter-arrival distribution: Markovian (0) or deterministic (1) |
| `type_size_k` | packet-size distribution: Markovian (0) or deterministic (1) |
| `source_k` | source id, which selects the result column (0 -> TS1, 1 -> TS2) |
| `priority_k` | queue the packets are filed into: low (0) or high (1) |

Simulated time is fixed at 100 s and the RNG seed at 2114 (`QSim.cc`), so runs are
reproducible. `Logger` **appends** to `results.csv`, so a 10-point load sweep is
produced by invoking the binary ten times in a row and then renaming the file.

To reproduce the analysis of the recorded results:

```bash
python analysis/analyze_results.py      # standard library only, no pandas needed
```

---

## How the file names were decoded

The 13 CSVs carry no metadata, only names. This is the reasoning used to recover
what each one is; the parts that are inference rather than fact are flagged.

**Columns** — certain. `QSim.cc` sets the header
`TS1_DELAY,TS2_DELAY,TS1_THROUGHPUT,TS2_THROUGHPUT`, and `Sink::Stop` writes
`aggregate_delay_HP/received_HP, aggregate_delay_LP/received_LP, aggregate_L_HP/SimTime, aggregate_L_LP/SimTime`,
with the HP/LP split keyed on `packet.source_id == 0`. So the columns are mean
delay [s] and delivered throughput [bps] for traffic source 1 and traffic source 2.

**Rows** — inference, but well supported. Every file has exactly 10 rows and
`Logger` appends one row per run. Column `TS1_THROUGHPUT` stays at ~100 kbps
throughout while `TS2_THROUGHPUT` climbs from ~1 Mbps to ~9.4 Mbps in near-equal
steps. So the swept parameter is `B_2`, taking 1, 2,... 10 Mbps, with
`B_1` = 100 kbps fixed.

**`np` / `p1` / `p2`** — inference, confirmed by the data. Compare the last row of
the three single-arrival files:

| file | TS1 delay | TS2 delay | reading |
| --- | --- | --- | --- |
| `ex1.csv` | 3.922 ms | 4.869 ms | both classes degrade together, and the 0.95 ms gap is exactly the difference in their transmission times — a single shared FIFO, **no priority** |
| `ex2.csv` | 1.050 ms | 5.258 ms | TS1 is held flat while TS2 absorbs the load — **priority to TS1** |
| `ex22.csv` | 24.641 ms | 5.124 ms | mirror image: TS2 is protected, TS1 explodes — **priority to TS2** |

The `ex3*np` / `p1` / `p2` triples show the same three signatures, so `np` = no
priority, `p1` = high priority given to traffic source 1, `p2` = high priority given
to traffic source 2. `ex1`/`ex2`/`ex22` are the same three policies run before the
batch source was introduced (their light-load TS2 delay is 1.20 ms, i.e. exactly one
transmission time, so `BATCHSIZE` was effectively 1 there).

**`4` / `8` / `16`** — this is the **batch size**, not the buffer size. Three
independent arguments:

1. *Direction of the throughput.* Delivered throughput **falls** as the number
   rises (class-2 loss at light load goes 3.1% -> 7.8% -> 42.5%). A larger *buffer*
   would reduce loss; a larger *burst* increases it. Only the batch reading fits.
2. *Quantitative match on the delay.* For a burst of *B* packets arriving at the
   same instant into an empty system, mean delay is `S·(B+1)/2` with `S` the
   transmission time. With the measured `S ≈ 1.20 ms` for class 2: B=4 predicts
   3.00 ms (measured 2.971), B=8 predicts 5.40 ms (measured 5.038, slightly low
   because packets are already being dropped), B=1 predicts 1.20 ms (measured
   1.203). The agreement is too close to be coincidence.
3. *The source code and the trace agree.* `TrafficSourceBatch.h` defines
   `#define BATCHSIZE 16` and `QSim.cc` includes that header — the file was left in
   the state of the last experiment, `ex316*`. `results/traces.txt` (written by that
   same last run) shows 16 packets generated at the identical timestamp
   `0.026301`, of which the last 6 are refused because the buffer is full.

**Buffer size** — inference from the trace, not from the CSVs. `traces.txt` shows
the buffer occupancy saturating at `HP=10`, so `Q = 10` packets for that run.

**`R`, `EL_1`, `EL_2`** — inference. At the lightest load, delay is essentially just
transmission time: 0.198 ms for class 1 and 1.203 ms for class 2. Total delivered
throughput tops out at ~9.5 Mbps. That is consistent with `R = 10 Mbps`,
`EL_1 ≈ 2000 bits`, `EL_2 ≈ 12000 bits`. **These three values are reconstructed
from the outputs, not read from a stored configuration**, and the exact command
lines used for the runs were not preserved.

**`example.csv`** is not an experiment — it is the two-column toy file
(`Value1,Value2`) shipped with the framework to demonstrate the logger, and it is
kept only for completeness.

---

## Results

All numbers below are produced by `analysis/analyze_results.py` from the recorded
CSVs in `results/`. Nothing here is estimated. `D1`/`D2` are mean delay per class,
`loss` is `1 − delivered / offered`.

### Effect of the priority policy, at saturation (`B_2` = 10 Mbps, ρ ≈ 1.01)

| batch | policy | D1 (ms) | D2 (ms) | loss 1 | loss 2 | total throughput (Mbps) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | none | 3.922 | 4.869 | 6.6% | 6.3% | 9.461 |
| 1 | TS1 | **1.050** | 5.258 | 0.3% | 5.5% | 9.550 |
| 1 | TS2 | 24.641 | **5.124** | 6.5% | 5.2% | 9.571 |
| 4 | none | 3.309 | 5.745 | 16.9% | 19.4% | 8.148 |
| 4 | TS1 | **1.030** | 6.003 | −1.4% | 18.3% | 8.266 |
| 4 | TS2 | 12.569 | **5.923** | 3.3% | 18.1% | 8.284 |
| 8 | none | 2.414 | 6.295 | 33.7% | 31.6% | 6.906 |
| 8 | TS1 | **1.096** | 6.440 | 2.5% | 30.9% | 7.005 |
| 8 | TS2 | 8.506 | **6.379** | 9.1% | 30.7% | 7.024 |
| 16 | none | 1.456 | 6.445 | 48.0% | 53.3% | 4.726 |
| 16 | TS1 | **0.951** | 6.511 | 31.3% | 52.9% | 4.774 |
| 16 | TS2 | 4.327 | **6.472** | 33.5% | 52.9% | 4.779 |

What this says:

- **Priority is close to free for the protected class and very expensive for the
  other one.** With single arrivals, giving priority to the light class (TS1) cuts
  its delay from 3.92 ms to 1.05 ms — a 3.7× improvement — while TS2's delay grows
  only from 4.87 ms to 5.26 ms (+8%). The asymmetry is entirely a consequence of the
  load ratio: TS1 offers 100 kbps against TS2's 10 Mbps, so promoting it barely
  perturbs the aggregate.
- **The mirror case shows what priority really costs.** Promoting the *heavy* class
  (TS2) buys it almost nothing (5.12 ms vs 4.87 ms without priority) while TS1's
  delay rises from 3.92 ms to 24.64 ms — a 6.3× penalty. Strict priority is only
  worth applying to a class that is small relative to the link.
- **Total throughput is essentially unaffected by the policy** (9.46 / 9.55 / 9.57
  Mbps for the single-arrival case). Priority reorders the queue; it does not
  create capacity. All the throughput variation across the table comes from the
  batch size, not from the discipline.
- **Priority also protects against loss, but only up to a point.** At batch 1 the
  protected class's loss drops from 6.6% to 0.3%. At batch 16 it only drops from
  48.0% to 31.3% — once bursts overflow a 10-packet buffer, no scheduling
  discipline can help, because the packet is dropped on arrival before scheduling
  is ever involved.

### Effect of burstiness (no-priority policy, so the discipline is held fixed)

| batch size | class-2 delay at light load | at saturation | class-2 loss at light load | at saturation |
| --- | --- | --- | --- | --- |
| 1 | 1.203 ms | 4.869 ms | −0.2% | 6.3% |
| 4 | 2.971 ms | 5.745 ms | 3.1% | 19.4% |
| 8 | 5.038 ms | 6.295 ms | 7.8% | 31.6% |
| 16 | 6.084 ms | 6.445 ms | 42.5% | 53.3% |

Burstiness is the dominant effect in the whole study. At *identical* offered load,
going from single arrivals to bursts of 16 costs 53% of the class-2 traffic and
drops total delivered throughput from 9.46 Mbps to 4.73 Mbps — half the link
wasted, with the queue idle between bursts and overflowing during them. Note also
that at batch 16 the delay curve is nearly flat (6.08 ms → 6.45 ms): the buffer is
saturated from the lightest load onwards, so extra offered traffic turns into loss
rather than into delay.

### Full load sweep, single arrivals, no priority (`ex1.csv`)

| `B_2` (Mbps) | ρ | D1 (ms) | D2 (ms) | Thr1 (kbps) | Thr2 (Mbps) |
| --- | --- | --- | --- | --- | --- |
| 1 | 0.11 | 0.198 | 1.203 | 100.2 | 1.002 |
| 2 | 0.21 | 0.327 | 1.314 | 99.7 | 1.975 |
| 3 | 0.31 | 0.491 | 1.493 | 100.0 | 2.985 |
| 4 | 0.41 | 0.730 | 1.730 | 100.0 | 4.006 |
| 5 | 0.51 | 0.970 | 1.985 | 99.0 | 4.999 |
| 6 | 0.61 | 1.326 | 2.347 | 98.4 | 5.943 |
| 7 | 0.71 | 1.791 | 2.756 | 99.6 | 6.874 |
| 8 | 0.81 | 2.475 | 3.422 | 98.5 | 7.818 |
| 9 | 0.91 | 3.152 | 4.192 | 96.4 | 8.717 |
| 10 | 1.01 | 3.922 | 4.869 | 93.4 | 9.367 |

The delay curve has the expected M/M/1-like shape: flat up to ρ ≈ 0.5, then rising
sharply as ρ approaches 1. Run `analysis/analyze_results.py` for the equivalent
sweeps of `ex2.csv` and `ex22.csv`.

---

## Limitations / honest notes

- **The Python analysis WAS run** (Python 3.11.9, standard library only) and the
  results tables above are its verbatim output.
- **The exact command lines used to produce the 13 CSVs were not preserved.** The
  parameter values quoted in "How the file names were decoded" (`R = 10 Mbps`,
  `EL_1 ≈ 2000` bits, `EL_2 ≈ 12000` bits, `Q = 10`, `B_2` swept 1→10 Mbps) are
  reconstructed from the outputs and the trace file. The reasoning is given above
  so it can be checked; the decode of `np`/`p1`/`p2` and of `4`/`8`/`16` as batch
  size is well supported, the precise numeric values of `R` and `EL_k` less so.
- **Small negative loss figures** (e.g. −0.2%, −1.4%) appear in the tables. These
  are sampling noise: packet sizes are drawn from an exponential distribution, so
  over a 100 s run the realised mean size can sit slightly above the nominal `EL`,
  making measured throughput marginally exceed the nominal offered load. They are
  reported as computed rather than clipped to zero.
- **`src/COST/` is third-party**, not the author's work. See `src/COST/README.md`.
  It is included because the project cannot be translated or compiled without it.
- **`QSim.cxx` and `compcxx_QSim.h` are generated** by `COST/cxx` from `QSim.cc`.
  They are gitignored and deliberately not shipped, so a `.cxx` step is required
  before any compilation.
- **The compiled `QSim` binary** from the original folder is not shipped
  (Linux ELF build output).
- **`stencil`-style scaling plots are not included** — the original project produced
  its plots outside this codebase and they were not preserved.
- **A course-wide lab report PDF exists** at
  `UNI-2DO-redes/Enginyeria de Xarxes/Lab Reports - Network Engineering - Google Docs.pdf`
  in the author's local coursework folder. It could not be read during packaging
  (no PDF tooling available) and is therefore **not** included here; it may contain
  the write-up of these experiments as well as unrelated labs.

## Authors

- José Mª Pérez Clar

No other student identifier appears anywhere in this project's files or folder
names. If this was submitted as group work, the co-author is not recorded in the
material that was packaged.

The COST framework under `src/COST/` is third-party; see `src/COST/README.md`.
