# GyroRank

**Elite adaptive multi-objective ranking kernel** with explicit gyroscopic strategy selection.

Derived from / hardened against the TDPSK Ranking Kernel lineage (pooled Fenwick, exact subquadratic 2-D ranking, zero-alloc spirit).

## Features

- Exact **O(N log N)** 2-objective weak-dominance ranking via `FenwickMax` (the reference)
- Adaptive `GyroController` (observe → striate → gate) that selects among exact kernels; it does not change ranks
- Observe is sampled (S ≤ 1024); full coordinate compression happens inside the chosen kernel once
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

// Explicit options
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
| 2 | Fenwick2D         | O(N log N)    | Θ(N)        | exact reference |
| ≥3| Projection        | O(N log N)    | Θ(N)        | first two objectives only |

## Architecture (gyroscopic view)

1. **Observe** – sampled sortedness and uniq hats (S ≤ 1024), O(min(N,S)). Full compress stays inside the kernel.
2. **Striate** – write dumpable U[k] for live strategies (named constexpr costs).
3. **Gate** – argmin U under exactness constraints.
4. **Attenuate** – run the chosen kernel once.

Exact O(N log N) 2-objective weak-dominance via FenwickMax is the reference.
GyroController selects among exact kernels. It does not change ranks.
Still only one exact 2-D kernel (Fenwick). LowAux2D was removed in Phase 2.
M≥3 is projection onto the first two objectives unless Phase 6 is done.
Approx1D is opt-in and inexact.
No claim of superiority to a hand-chosen Fenwick call on all inputs.
No Lyapunov, no trading alpha, no χ, no second algebraic substrate.

Compile with `-DGYRO_DEBUG` to dump features, U[], and chosen strategy to stderr. Release builds are silent.

## License

MIT

## Credits

TDPSK Ranking Kernel templates · Orson Peters pdqsort · Gyroscopic adaptive framing
