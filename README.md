# GyroRank

Exact 2-D Fenwick weak-dominance ranking. One header. M=1 is layer rank. M≥2 is FenwickMax + FenwickSum on the first two objectives. Identity tests compare `execute_gyro_rank` to `exact_rank_2d_fenwick`. There is no controller. A selector that cannot change ranks is not a part.

Derived from the TDPSK Ranking Kernel lineage (pooled Fenwick, exact subquadratic 2-D ranking).

## Quick Start

```bash
# Optional: official Orson Peters pdqsort on the include path
curl -sL https://raw.githubusercontent.com/orlp/pdqsort/master/pdqsort.h -o pdqsort.h

g++ -O3 -std=c++17 -march=native -Iinclude examples/demo.cpp -o demo
./demo

g++ -O2 -std=c++17 -Iinclude tests/identity.cpp -o identity && ./identity
```

## Usage

```cpp
#include "gyro_rank.hpp"

std::vector<double> matrix(n * m);   // row-major, n points × m objectives
std::vector<int32_t> ranks(n), dom(n);

gyro::execute_gyro_rank(matrix.data(), n, m, ranks.data(), dom.data());
```

## Complexity

| M | Path      | Time       | Aux space | Notes |
|---|-----------|------------|-----------|-------|
| 1 | Rank1D    | O(N log N) | O(N)      | exact |
| ≥2 | Fenwick2D | O(N log N) | Θ(N)      | first two objectives |

No Lyapunov. No trading alpha. No χ. No second algebraic substrate. No claim of superiority to a hand-chosen Fenwick call.

## License

**Dual licensed.**

- Non-commercial / research / evaluation / non-production → [AGPLv3](LICENSE-AGPL)
- Commercial / production / embedding / SaaS / redistribution as product → requires a [paid proprietary Commercial License](LICENSE-COMMERCIAL)

See [LICENSE](LICENSE) and [COMMERCIAL.md](COMMERCIAL.md).

Until a commercial grant is issued in writing, AGPLv3 governs all use.

Copyright (c) 2026 Heywood Geblomi.

## Credits

TDPSK Ranking Kernel templates · Orson Peters pdqsort
