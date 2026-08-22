# GYR-AXX-001 Track 1 — Blunck & Vahrenhold Layers of Maxima

**Branch:** `gyr-axx-track1` (exclusive)  
**Status:** Commit 1 — citation + empty stub + identity harness only

## Family (named, legal)

Blunck, H. & Vahrenhold, J.  
*In-Place Algorithms for Computing (Layers of) Maxima*  
Algorithmica 57, 1–21 (2010)  
DOI: [10.1007/s00453-008-9193-z](https://doi.org/10.1007/s00453-008-9193-z)

- 2-D layers of maxima
- O(n log n) time
- O(1) extra words (true in-place)
- Distinct from SFS / active-front and from sqrt-decomposition (both previously failed Hydra and deleted)

## Sense mapping (critical)

The paper defines maxima with the “larger in all coordinates” convention (higher-is-better).  
GyroRank / Fenwick uses **lower-is-better** weak dominance for ranks.

**Contract:** flip both axes (negate or reverse-order map) so that the paper’s maxima become our minima. After the flip the layer ranks must be bit-identical to `exact_rank_2d_fenwick`.

## Input / output contract (critical)

- GyroRank identity requires ranks written in the **caller’s original point order**.
- The pure Blunck–Vahrenhold algorithm permutes its array.
- Therefore we must either:
  1. Rank in-place then unpermute back to original order, or
  2. Write ranks without destroying the caller’s buffer (temporary O(n) is allowed only if H-MEM still remains plausible).

Commit 1 does not yet choose; the choice must be stated in the first implementation commit.

## Equal-x + stable ties

Fenwick processes equal-x groups together (query all, then update all) and breaks remaining ties by stable original index.  
Any layers implementation must reproduce the identical contract or identity will fail.

## Hydra target

- **H-MEM** is the only plausible class (O(1) extra space is the selling point).
- Do **not** expect a win on H-DOM.
- Win condition remains wall-clock ≤ 0.70× Fenwick at N=10⁶, best-of-3, g++ -O2.
- No win → delete the kernel and the Strategy entry in the same PR. Deletion with measured numbers is a clean Track 1 exit (not A++).

## Commit 1 contents

- This document
- Empty / forwarding stub `exact_rank_2d_layers` (currently calls Fenwick so identity stays green)
- Identity harness that forces the new path against `exact_rank_2d_fenwick`
- **No** Strategy enum change
- **No** gate / striate wiring

## Non-goals (still frozen)

- χ, Prym, GeblomiSort stay out of `gyro_rank.hpp`
- No Kung peeling
- Track 2 / Track 3 require an explicit GO after Track 1 closes
