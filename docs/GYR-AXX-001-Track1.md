# GYR-AXX-001 Track 1 — Blunck & Vahrenhold Layers of Maxima

**Branch:** `gyr-axx-track1` (exclusive)  
**Status:** Commit 2 — real layers kernel (identity green, no Fenwick call)

## Family (named, legal)

Blunck, H. & Vahrenhold, J.  
*In-Place Algorithms for Computing (Layers of) Maxima*  
Algorithmica 57, 1–21 (2010)  
DOI: [10.1007/s00453-008-9193-z](https://doi.org/10.1007/s00453-008-9193-z)

- 2-D layers of maxima
- O(n log n) time (paper); current Commit-2 extraction is O(n · L) for identity clarity
- O(1) extra words in the pure paper algorithm; we use an index permutation (unpermute)
- Distinct from SFS / active-front and from sqrt-decomposition

## Sense mapping

Paper: maximality = “larger in all coordinates” (higher-is-better).  
GyroRank / Fenwick: **lower-is-better** weak dominance.

Commit-2 implementation works directly in the lower-is-better convention (no explicit negate), which is equivalent after the documented flip. Ranks are defined by successive extraction of the non-dominated front under:

```
q dominates p  iff  q.x ≤ p.x ∧ q.y ≤ p.y ∧ (q.x < p.x ∨ q.y < p.y)
```

Identical coordinates do not dominate each other → same layer (matches Fenwick equal-batch).

## Input / output contract — **unpermute** (locked in Commit 2)

- Caller’s matrix is **never modified**.
- Only an index vector (`remaining`) is permuted / filtered.
- Ranks are written directly into `ranks_out[original_index]`.
- This preserves GyroRank identity (ranks in caller order) while avoiding a full point-array copy.
- Extra space: O(n) indices (acceptable for identity; H-MEM evaluation is Commit 3+).

## Equal-x + stable ties

- Points with equal objectives receive the same layer (neither dominates).
- Strict inequality on at least one objective is required for domination.
- Matches Fenwick’s equal-x batch behaviour (query-all then update-all).

## Hydra target (Commit 3+, not this commit)

- **H-MEM** is the only plausible class.
- Win = wall ≤ 0.70× Fenwick at N=10⁶, best-of-3, g++ -O2.
- No win → delete kernel + any Strategy entry in the same PR.

## Commit history

| Commit | Contents |
|--------|----------|
| 1 | Citation + empty stub (forwarded to Fenwick) + identity harness |
| 2 | Real layers kernel (this); unpermute contract locked; no Strategy |

## Non-goals (still frozen)

- χ, Prym, GeblomiSort stay out of `gyro_rank.hpp`
- No Kung peeling
- No Strategy / gate / striate wire
- Track 2 / Track 3 require explicit GO after Track 1 closes
