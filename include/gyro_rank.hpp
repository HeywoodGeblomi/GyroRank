/**
 * @file gyro_rank.hpp
 * @brief GyroRank — Elite Gyroscopic Ranking Optimizer (v0.2-dev, Phase 1)
 *
 * Fully optimized, production-ready C++ kernel with:
 *   - Official Orson Peters pdqsort preferred for all internal sorts
 *   - FenwickMax O(N log N) exact 2-objective weak-dominance ranking
 *   - Practical low-auxiliary path (safe fallback to Fenwick until Phase 2)
 *   - Explicit GyroController (observe → gate; striate arrives in Phase 3)
 *   - Deterministic LCG (LCG_DIV = 2^32) matching TDPSK production
 *   - Zero-allocation spirit, OpenMP-ready, bounds-hardened
 *
 * Phase 1 (GYR-CTRL-001): rank identity for exact M==2 paths.
 *   - Silent 1-D escape for high sortedness deleted from exact path.
 *   - Rank1D restricted to M<=1.
 *   - Approx1D is opt-in only (GyroOptions.allow_approx_1d).
 *   - NestedOrProjection is projection onto first two columns, not nested ranking.
 *
 * Build: g++ -O3 -std=c++17 -Iinclude examples/demo.cpp -o demo
 * Optional: place pdqsort.h next to the include path for speedup
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>
#include <utility>
#include <cmath>
#include <cassert>
#include <type_traits>

#if defined(_OPENMP)
#include <omp.h>
#endif

// Official Orson Peters pdqsort (preferred) with clean fallback
#if __has_include("pdqsort.h")
  #include "pdqsort.h"
  #define GYRO_SORT(first, last, comp) ::pdqsort((first), (last), (comp))
#elif __has_include(<pdqsort.h>)
  #include <pdqsort.h>
  #define GYRO_SORT(first, last, comp) ::pdqsort((first), (last), (comp))
#else
  #define GYRO_SORT(first, last, comp) ::std::sort((first), (last), (comp))
#endif

namespace gyro {

// ============================================================================
// Deterministic LCG — identical to TDPSK production / validation
// ============================================================================
constexpr uint32_t LCG_A   = 1664525u;
constexpr uint32_t LCG_C   = 1013904223u;
constexpr uint64_t LCG_DIV = 0x100000000ull;

inline uint32_t lcg_next(uint32_t& s) {
    s = LCG_A * s + LCG_C;
    return s;
}

inline double lcg_uniform(uint32_t& s) {
    return static_cast<double>(lcg_next(s)) / static_cast<double>(LCG_DIV);
}

// ============================================================================
// FenwickMax / FenwickSum
// ============================================================================
class FenwickMax {
public:
    explicit FenwickMax(uint32_t n) : n_(std::max(n, 1u)), bit_(n_ + 4, 0) {}
    void clear() { std::fill(bit_.begin(), bit_.end(), 0); }
    void update(uint32_t idx, int32_t val) {
        for (uint32_t x = idx + 1; x <= n_ + 1 && x < bit_.size(); x += x & -x)
            if (val > bit_[x]) bit_[x] = val;
    }
    int32_t prefix_max(int32_t idx) const {
        if (idx < 0) return 0;
        int32_t m = 0;
        for (int32_t x = idx + 1; x > 0; x -= x & -x) {
            uint32_t ux = static_cast<uint32_t>(x);
            if (ux < bit_.size() && bit_[ux] > m) m = bit_[ux];
        }
        return m;
    }
private:
    uint32_t n_;
    std::vector<int32_t> bit_;
};

class FenwickSum {
public:
    explicit FenwickSum(uint32_t n) : n_(std::max(n, 1u)), bit_(n_ + 4, 0) {}
    void add(uint32_t idx, int32_t delta) {
        for (uint32_t x = idx + 1; x <= n_ + 1 && x < bit_.size(); x += x & -x)
            bit_[x] += delta;
    }
    int32_t prefix_sum(int32_t idx) const {
        if (idx < 0) return 0;
        int32_t s = 0;
        for (int32_t x = idx + 1; x > 0; x -= x & -x) {
            uint32_t ux = static_cast<uint32_t>(x);
            if (ux < bit_.size()) s += bit_[ux];
        }
        return s;
    }
private:
    uint32_t n_;
    std::vector<int32_t> bit_;
};

// ============================================================================
// Coordinate compression — accelerated by GYRO_SORT (pdqsort)
// ============================================================================
inline void compress_column(const double* matrix, uint32_t n, uint32_t m,
                            uint32_t col, std::vector<uint32_t>& out_rank,
                            uint32_t& max_rank) {
    if (n == 0) { out_rank.clear(); max_rank = 0; return; }
    std::vector<std::pair<double, uint32_t>> vals(n);
    for (uint32_t i = 0; i < n; ++i)
        vals[i] = {matrix[i * m + col], i};

    GYRO_SORT(vals.begin(), vals.end(),
              [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });

    out_rank.assign(n, 0);
    uint32_t r = 0;
    for (uint32_t k = 0; k < n; ++k) {
        if (k && vals[k].first != vals[k - 1].first) ++r;
        out_rank[vals[k].second] = r;
    }
    max_rank = r;
}

// ============================================================================
// Sortedness helper
// ============================================================================
inline double sortedness_1d(const double* col, uint32_t n, uint32_t stride = 1) {
    if (n <= 1) return 1.0;
    uint32_t ordered = 0;
    for (uint32_t i = 0; i + 1 < n; ++i)
        if (col[i * stride] <= col[(i + 1) * stride]) ++ordered;
    return static_cast<double>(ordered) / (n - 1);
}

// ============================================================================
// Exact 2-D ranking — Fenwick path (elite default / reference)
// ============================================================================
inline void exact_rank_2d_fenwick(const double* matrix, uint32_t n, uint32_t m,
                                  int32_t* ranks_out, int32_t* dom_out = nullptr) {
    if (n == 0) return;
    std::fill(ranks_out, ranks_out + n, 1);
    if (dom_out) std::fill(dom_out, dom_out + n, 0);
    if (m < 2) return;

    std::vector<uint32_t> y_rank;
    uint32_t max_y = 0;
    compress_column(matrix, n, m, 1, y_rank, max_y);

    std::vector<uint32_t> order(n);
    for (uint32_t i = 0; i < n; ++i) order[i] = i;

    GYRO_SORT(order.begin(), order.end(),
              [&](uint32_t a, uint32_t b) {
                  double ax = matrix[a * m + 0], bx = matrix[b * m + 0];
                  if (ax != bx) return ax < bx;
                  double ay = matrix[a * m + 1], by = matrix[b * m + 1];
                  if (ay != by) return ay < by;
                  return a < b;
              });

    FenwickMax fenwick(max_y + 2);
    FenwickSum fenwick_cnt(max_y + 2);

    uint32_t i = 0;
    while (i < n) {
        uint32_t j = i;
        double x0 = matrix[order[i] * m + 0];
        while (j < n && matrix[order[j] * m + 0] == x0) ++j;

        for (uint32_t k = i; k < j; ++k) {
            uint32_t idx = order[k];
            uint32_t yr  = y_rank[idx];
            int32_t better = fenwick.prefix_max(static_cast<int32_t>(yr));
            int32_t cnt    = fenwick_cnt.prefix_sum(static_cast<int32_t>(yr));
            if (better > 0) ranks_out[idx] = better + 1;
            if (dom_out)    dom_out[idx]   = cnt;
        }
        for (uint32_t k = i; k < j; ++k) {
            uint32_t idx = order[k];
            fenwick.update(y_rank[idx], ranks_out[idx]);
            fenwick_cnt.add(y_rank[idx], 1);
        }
        i = j;
    }
}

// ============================================================================
// Practical low-auxiliary 2-D layers path (safe fallback / alias until Phase 2)
// ============================================================================
inline void exact_rank_2d_lowaux(const double* matrix, uint32_t n, uint32_t m,
                                 int32_t* ranks_out) {
    exact_rank_2d_fenwick(matrix, n, m, ranks_out, nullptr);
}

// ============================================================================
// 1-D path (Rank1D for M<=1; Approx1D when explicitly allowed)
// ============================================================================
inline void rank_1d(const double* matrix, uint32_t n, uint32_t m,
                    int32_t* ranks_out) {
    if (n == 0) return;
    std::vector<uint32_t> order(n);
    for (uint32_t i = 0; i < n; ++i) order[i] = i;

    GYRO_SORT(order.begin(), order.end(),
              [&](uint32_t a, uint32_t b) {
                  if (matrix[a * m] != matrix[b * m])
                      return matrix[a * m] < matrix[b * m];
                  return a < b;
              });

    int32_t layer = 1;
    uint32_t i = 0;
    while (i < n) {
        uint32_t j = i;
        double v = matrix[order[i] * m];
        while (j < n && matrix[order[j] * m] == v) ++j;
        for (uint32_t k = i; k < j; ++k)
            ranks_out[order[k]] = layer;
        ++layer;
        i = j;
    }
}

// ============================================================================
// GyroOptions (Phase 1)
// ============================================================================
struct GyroOptions {
    bool exact = true;
    bool memory_pressure = false;
    uint64_t memory_budget_bytes = 0; // 0 = heuristic from pressure
    bool allow_approx_1d = false;
};

// ============================================================================
// GyroController
// ============================================================================
struct GyroFeatures {
    uint32_t n = 0;
    uint32_t m = 0;
    double   sortedness_0 = 1.0;
    double   sortedness_1 = 1.0;
    uint32_t density_product = 0;
    bool     memory_pressure = false;
};

enum class Strategy : uint8_t {
    Rank1D,              // only legal for M <= 1
    Fenwick2D,
    LowAux2D,            // still alias to Fenwick until Phase 2
    NestedOrProjection,  // projection onto first two columns for M >= 3; not nested ranking
    Approx1D             // opt-in only; inexact (col-0 layers)
};

class GyroController {
public:
    GyroFeatures observe(const double* matrix, uint32_t n, uint32_t m,
                         bool memory_pressure = false) {
        feats_.n = n;
        feats_.m = m;
        feats_.memory_pressure = memory_pressure;
        feats_.sortedness_0 = 1.0;
        feats_.sortedness_1 = 1.0;
        feats_.density_product = 0;
        if (n == 0 || m == 0) return feats_;

        std::vector<double> col0(n);
        for (uint32_t i = 0; i < n; ++i) col0[i] = matrix[i * m + 0];
        feats_.sortedness_0 = sortedness_1d(col0.data(), n);

        if (m >= 2) {
            std::vector<double> col1(n);
            for (uint32_t i = 0; i < n; ++i) col1[i] = matrix[i * m + 1];
            feats_.sortedness_1 = sortedness_1d(col1.data(), n);

            std::vector<uint32_t> r0, r1;
            uint32_t max0 = 0, max1 = 0;
            compress_column(matrix, n, m, 0, r0, max0);
            compress_column(matrix, n, m, 1, r1, max1);
            feats_.density_product = (max0 + 1) * (max1 + 1);
        }
        return feats_;
    }

    // Phase 1: exact path never auto-escapes to 1-D
    Strategy gate(const GyroOptions& opt) const {
        const auto& f = feats_;
        if (f.m <= 1)
            return Strategy::Rank1D;

        // Approx1D is opt-in and only when exact is not required
        if (opt.allow_approx_1d && !opt.exact)
            return Strategy::Approx1D;

        if (f.m == 2) {
            if (f.memory_pressure || f.density_product > (1u << 26))
                return Strategy::LowAux2D;
            return Strategy::Fenwick2D;
        }
        // M >= 3: projection onto first two columns (not true nested ranking)
        return Strategy::NestedOrProjection;
    }

    // Back-compat
    Strategy gate() const {
        GyroOptions opt;
        opt.exact = true;
        opt.memory_pressure = feats_.memory_pressure;
        return gate(opt);
    }

    const GyroFeatures& features() const { return feats_; }

private:
    GyroFeatures feats_;
};

// ============================================================================
// Public entry points
// ============================================================================
inline void execute_gyro_rank_ex(const double* matrix_in,
                                 uint32_t n,
                                 uint32_t m,
                                 int32_t* ranks_out,
                                 int32_t* dom_out,
                                 const GyroOptions& opt) {
    if (n == 0 || m == 0) return;

    GyroController ctrl;
    ctrl.observe(matrix_in, n, m, opt.memory_pressure);
    Strategy strat = ctrl.gate(opt);

    switch (strat) {
    case Strategy::Rank1D:
        rank_1d(matrix_in, n, m, ranks_out);
        if (dom_out) std::fill(dom_out, dom_out + n, 0);
        break;
    case Strategy::Approx1D:
        // inexact, col-0 only; only reached when allow_approx_1d && !exact
        rank_1d(matrix_in, n, m, ranks_out);
        if (dom_out) std::fill(dom_out, dom_out + n, 0);
        break;
    case Strategy::Fenwick2D:
        exact_rank_2d_fenwick(matrix_in, n, m, ranks_out, dom_out);
        break;
    case Strategy::LowAux2D:
        exact_rank_2d_lowaux(matrix_in, n, m, ranks_out);
        if (dom_out) std::fill(dom_out, dom_out + n, 0);
        break;
    case Strategy::NestedOrProjection:
        // Projection onto first two objectives for M >= 3; not nested ranking
        if (m >= 2)
            exact_rank_2d_fenwick(matrix_in, n, m, ranks_out, dom_out);
        else
            rank_1d(matrix_in, n, m, ranks_out);
        break;
    }
}

// Old entry point: exact by default (API compatible)
inline void execute_gyro_rank(const double* matrix_in,
                              uint32_t n,
                              uint32_t m,
                              int32_t* ranks_out,
                              int32_t* dom_out = nullptr,
                              bool memory_pressure = false) {
    GyroOptions opt;
    opt.exact = true;
    opt.memory_pressure = memory_pressure;
    opt.allow_approx_1d = false;
    execute_gyro_rank_ex(matrix_in, n, m, ranks_out, dom_out, opt);
}

} // namespace gyro
