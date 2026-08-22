# GYR-AXX-002 Track 2 — Fenwick reference table

**Branch:** `gyr-axx-track2`  
**Status:** Reference evidence only. No new kernel. Header stays Fenwick-only.  
**Grade:** B+ / A− evidence. **Not A++.**  
**promote_ready:** false. Track 3 frozen.

## What this is

A published-style wall-time table for exact 2-D weak-dominance layers (lower-is-better, same tie rules as Fenwick).  
Track 1 deleted the successive peeler. This GO is **evidence**, not a second algorithm.

## Host and protocol

- Host: linux (CI-class / this sandbox)
- Compiler style: g++ -O2, C++17
- Wall: best-of-3
- Product path timed: `exact_rank_2d_fenwick` only
- Generators (pinned): random seed=42, sorted-x random-y, high-L / H-DOM
- N ∈ {1e5, 1e6}. N=1e7 not run (job budget).

## Results

| Generator | N | Fenwick (ms) | BNL (ms) | Skyline-only (ms) |
|-----------|---|-------------:|---------:|------------------:|
| random (seed=42) | 1e5 | 51.4 | skip O(N²) | 12.8 |
| random (seed=42) | 1e6 | 877.0 | skip O(N²) | 325.5 |
| sorted-x random-y | 1e5 | 20.2 | skip O(N²) | 3.1 |
| sorted-x random-y | 1e6 | 364.4 | skip O(N²) | 39.5 |
| high-L / H-DOM | 1e5 | 31.8 | skip O(N²) | 9.1 |
| high-L / H-DOM | 1e6 | 326.3 | skip O(N²) | 71.5 |

### BNL sample (identity-checked)

| N | BNL (ms) | Fenwick (ms) | Ratio |
|---|---------:|-------------:|------:|
| 2000 | 1395.0 | 0.4 | ~3700× |

BNL vs Fenwick identity (N=512, 2048): **PASS** (bit-identical ranks).  
BNL at N=1e5 / 1e6: **skipped** — O(N²) exceeds practical CI budget. That is the point of the table.

## Labels (honesty)

| Path | Full layers? | Identity vs Fenwick? | Notes |
|------|--------------|----------------------|-------|
| `exact_rank_2d_fenwick` | Yes | Reference | Product path |
| BNL / nested | Yes | Yes (verified small-N) | O(N²); sample only |
| Skyline-only sweep | **No** (rank-1 membership) | **No** — labeled inexact | sort-x + one-pass y-best |

## What this does **not** claim

- No “beats NSGA.”
- No M≥3.
- No χ / Prym / GeblomiSort in the header.
- No Strategy change.
- No second kernel.
- A++ only after external reproduction (out of scope here).

## Acceptance (R1–R5)

| ID | Pass |
|----|------|
| R1 | `tests/ref_bench.cpp` builds and prints N=1e5 / N=1e6 Fenwick times |
| R2 | This table matches that run (host noted above) |
| R3 | BNL present at sample N; 1e5/1e6 skipped with reason |
| R4 | Sweep labeled skyline-only / inexact for layers |
| R5 | `gyro_rank.hpp` Strategy unchanged |

## Done

R1–R5 on main. Stop. Do not start Track 3 or another kernel.
