# GyroRank

**Elite adaptive multi-objective ranking kernel** with explicit gyroscopic strategy selection.

Derived from / hardened against the TDPSK Ranking Kernel lineage (pooled Fenwick, exact subquadratic 2-D ranking, zero-alloc spirit).  
Observes data character (N, M, sortedness, density) → gates between high-throughput Fenwick paths and low-auxiliary strategies.

## Features

- Exact **O(N log N)** 2-objective weak-dominance ranking via `FenwickMax`
- Adaptive `GyroController` (observe → striate → gate)
- All internal coordinate / order sorts prefer **official Orson Peters pdqsort** (falls back cleanly to `std::sort`)
- Deterministic LCG identical to TDPSK production (`A=1664525`, `C=1013904223`, `DIV=2^32`)
- Single-header, bounds-hardened, zero-allocation spirit
- 1-D ranking path + practical low-aux fallback (currently safe map to Fenwick)

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

gyro::execute_gyro_rank(matrix.data(), n, m,
                        ranks.data(), dom.data(),
                        /*memory_pressure=*/false);
```

## Complexity

| M | Path              | Time          | Aux space   |
|---|-------------------|---------------|-------------|
| 1 | rank_1d           | O(N log N)    | O(N)        |
| 2 | Fenwick (elite)   | O(N log N)    | Θ(N)        |
| 2 | Low-aux (safe)    | O(N log N)    | Θ(N)        |
| ≥3| projection note   | —             | —           |

## Architecture (gyroscopic view)

1. **Observe** – sortedness per objective, density product after compression, N/M, memory flag
2. **Striate** – rank candidate strategies by utility
3. **Gate** – select Insertion1D / Fenwick2D / LowAux2D / NestedOrProjection
4. **Attenuate** – run the chosen kernel

This is the computational analogue of a recursive trucker's hitch: apply a high-leverage primitive, observe diminishing returns, switch strategy.

## License

MIT

## Credits

TDPSK Ranking Kernel templates · Orson Peters pdqsort · Gyroscopic adaptive framing
