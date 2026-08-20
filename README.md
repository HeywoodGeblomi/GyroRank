# GyroRank

**Elite adaptive multi-objective ranking kernel** with explicit gyroscopic strategy selection.

Derived from / hardened against the TDPSK Ranking Kernel lineage (pooled Fenwick, exact subquadratic 2-D ranking, zero-alloc spirit).

## Features

- Exact **O(N log N)** 2-objective weak-dominance ranking via `FenwickMax` (the reference)
- Adaptive `GyroController` that selects among exact kernels; it does not change ranks
- All internal coordinate / order sorts prefer **official Orson Peters pdqsort** (falls back cleanly to `std::sort`)
- Deterministic LCG identical to TDPSK production (`A=1664525`, `C=1013904223`, `DIV=2^32`)
- Single-header, bounds-hardened, zero-allocation spirit
- `Approx1D` is opt-in and inexact; default path is exact
- M ≥ 3 is projection onto the first two objectives (not nested ranking) unless Phase 6 ships a real DC kernel

## Quick Start

```bash
# Optional but recommended for max sort performance
curl -sL https://raw.githubusercontent.com/orlp/pdqsort/master/pdqsort.h -o pdqsort.h

g++ -O3 -std=c++17 -march=native -Iinclude examples/demo.cpp -o demo
./demo
```

## Usage

```cpp
#include "gyro_rank.hpp"

std::vector<double> matrix(n * m);   // row-major, n points × m objectives
std::vector<int32_t> ranks(n), dom(n);

// Default: exact
gyro::execute_gyro_rank(matrix.data(), n, m,
                        ranks.data(), dom.data(),
                        /*memory_pressure=*/false);

// Explicit options (Phase 1+)
gyro::GyroOptions opt;
opt.exact = true;
opt.allow_approx_1d = false;
opt.memory_pressure = false;
gyro::execute_gyro_rank_ex(matrix.data(), n, m,
                           ranks.data(), dom.data(), opt);
```

## Complexity

| M | Path              | Time          | Aux space   | Notes |
|---|-------------------|---------------|-------------|-------|
| 1 | Rank1D            | O(N log N)    | O(N)        | exact |
| 2 | Fenwick (elite)   | O(N log N)    | Θ(N)        | exact reference |
| ≥3| Projection        | O(N log N)    | Θ(N)        | first two objectives only |

## Architecture (gyroscopic view)

1. **Observe** – sortedness per objective, density product after compression, N/M, memory flag
2. **Striate** – (Phase 3) rank candidate strategies by utility
3. **Gate** – select Rank1D / Fenwick2D / NestedOrProjection / Approx1D subject to exactness and budget
4. **Attenuate** – run the chosen kernel once

Exact O(N log N) 2-objective weak-dominance via FenwickMax is the reference.
GyroController selects among exact kernels. It does not change ranks.
LowAux2D was removed in Phase 2 (no RSS-winning distinct exact kernel was ready; stub not allowed).
M≥3 is projection onto the first two objectives unless Phase 6 is done.
Approx1D is opt-in and inexact.
No claim of superiority to a hand-chosen Fenwick call on all inputs.
No Lyapunov, no trading alpha, no χ, no second algebraic substrate.

## License

MIT

## Credits

TDPSK Ranking Kernel templates · Orson Peters pdqsort · Gyroscopic adaptive framing
