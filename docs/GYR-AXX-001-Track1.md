# GYR-AXX-001 Track 1 — Successive Layers Extraction

**Branch:** `gyr-axx-track1` (exclusive)  
**Status:** Commit 2 (revised) — successive front extraction (Kung-style peeling), identity green

## Named algorithm (honest)

**Successive front extraction** (also called Kung-style peeling / successive layers of minima).

- Repeatedly extract the current non-dominated front under lower-is-better weak dominance.
- Sort remaining points by (x↑, y↑, index↑), peel the front via a min-y envelope, assign the current layer, remove the front, repeat.
- Complexity: O(n · L) where L is the number of layers.
- Extra space: O(n) for the index vector of remaining points (unpermute contract).
- **Not** Blunck & Vahrenhold, Algorithmica 2010 (that paper is true in-place O(n log n) with O(1) extra words).
- **Not** SFS / active-front, not sqrt-decomposition.

The ticket explicitly marked pure Kung peeling as off / unlikely versus Fenwick. This kernel is kept only long enough for an honest Hydra measurement and expected deletion.

## Sense mapping

GyroRank / Fenwick use **lower-is-better** weak dominance:

```
q dominates p  iff  q.x ≤ p.x ∧ q.y ≤ p.y ∧ (q.x < p.x ∨ q.y < p.y)
```

Identical coordinates do not dominate each other → same layer (matches Fenwick equal-x batch behaviour).

## Input / output contract — **unpermute**

- Caller’s matrix is **never modified**.
- Only an index vector (`remaining`) is sorted / filtered.
- Ranks are written directly into `ranks_out[original_index]`.
- Extra space is Θ(n) indices. This already makes a true H-MEM win improbable.

## Equal-x + stable ties

- Equal-x groups are processed together.
- Ties on both coordinates are broken by original index (stable).
- Matches Fenwick’s query-before-update equal-x contract.

## Hydra target (next commit)

- Win condition remains wall ≤ 0.70× Fenwick at N = 10⁶, best-of-3, g++ -O2.
- **Expectation:** the peeler will lose badly on both H-DOM and H-MEM except for pathological tiny-L inputs.
- No win → delete the kernel (and any Strategy entry if one had been added) in the same PR. Deletion with measured numbers is a clean Track 1 exit (not A++).

## Commit history

| Commit | Contents |
|--------|----------|
| 1 | Citation scaffolding + empty stub + identity harness |
| 2 (revised) | Real successive-layers peeler under its true name; BV claim removed; header restored |

## Non-goals (still frozen)

- χ, Prym, GeblomiSort stay out of `gyro_rank.hpp`
- No Strategy / gate / striate wire
- Track 2 / Track 3 require an explicit GO after Track 1 closes
- No false citation of Algorithmica 2010 for this code
