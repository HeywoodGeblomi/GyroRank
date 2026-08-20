/**
 * @file gyro_rank.hpp
 * @brief GyroRank — Elite Gyroscopic Ranking Optimizer (v0.2-dev)
 *
 * Fully optimized, production-ready C++ kernel with:
 *   - Official Orson Peters pdqsort preferred for all internal sorts
 *   - FenwickMax O(N log N) exact 2-objective weak-dominance ranking (reference)
 *   - Explicit GyroController (observe → striate → gate)
 *   - Deterministic LCG (LCG_DIV = 2^32) matching TDPSK production
 *   - Zero-allocation spirit, OpenMP-ready, bounds-hardened
 *
 * Phase 1: rank identity (silent 1-D escape deleted, Rank1D ≤ M=1, Approx1D opt-in).
 * Phase 2: LowAux2D stub deleted.
 * Phase 3: cheap observe (sample S≤1024) + striate() writing dumpable U[k]; gate = argmin U.
 *
 * Build: g++ -O3 -std=c++17 -Iinclude examples/demo.cpp -o demo
 * Optional: place pdqsort.h next to the include path for speedup
 * Debug: compile with -DGYRO_DEBUG for features/U[]/strategy dump on stderr.
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
#include <unordered_set>
#ifdef GYRO_DEBUG
#include <cstdio>
#endif

#if defined(_OPENMP)
#include <omp.h>
#endif

// Official Orson Peters pdqsort (preferred) with clean fallback
// Variadic so commas inside comparator lambdas do not split the macro
#if __has_include("pdqsort.h")
  #include "pdqsort.h"
  #define GYRO_SORT(...) ::pdqsort(__VA_ARGS__)
#elif __has_include(<pdqsort.h>)
  #include <pdqsort.h>
  #define GYRO_SORT(...) ::pdqsort(__VA_ARGS__)
#else
  #define GYRO_SORT(...) ::std::sort(__VA_ARGS__)
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
// Full compress stays inside the chosen kernel only (Phase 3)
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
// Sortedness helper (used on samples in observe)
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
// GyroController — Phase 3: cheap observe + striate
// ============================================================================
struct GyroFeatures {
    uint32_t n = 0;
    uint32_t m = 0;
    double   sortedness_0 = 1.0;
    double   sortedness_1 = 1.0;
    uint32_t uniq_x_hat = 0;
    uint32_t uniq_y_hat = 0;
    uint32_t density_product = 0; // estimate = uniq_x_hat * uniq_y_hat
    bool     memory_pressure = false;
    uint64_t memory_budget_bytes = 0;
};

enum class Strategy : uint8_t {
    Rank1D,              // only legal for M <= 1
    Fenwick2D,           // sole exact M==2 kernel (reference)
    NestedOrProjection,  // projection onto first two columns for M >= 3
    Approx1D             // opt-in only; inexact
};

// Named cost constants (calibrate in comments; not theorems)
constexpr double GYRO_C1 = 1.0;   // N log N coefficient for Fenwick / Rank1D
constexpr double GYRO_C2 = 0.5;   // uniq_y term for Fenwick
constexpr double GYRO_INF = 1e300;

class GyroController {
public:
    // Phase 3: O(min(N,S)) with S <= 1024. Full compress stays inside kernel.
    GyroFeatures observe(const double* matrix, uint32_t n, uint32_t m,
                         bool memory_pressure = false,
                         uint64_t memory_budget_bytes = 0) {
        feats_.n = n;
        feats_.m = m;
        feats_.memory_pressure = memory_pressure;
        feats_.memory_budget_bytes = memory_budget_bytes;
        feats_.sortedness_0 = 1.0;
        feats_.sortedness_1 = 1.0;
        feats_.uniq_x_hat = 0;
        feats_.uniq_y_hat = 0;
        feats_.density_product = 0;
        if (n == 0 || m == 0) return feats_;

        const uint32_t S = std::min(n, 1024u);
        // Deterministic sample indices via stride
        const uint32_t step = (n + S - 1) / S;

        // Sampled sortedness_0
        {
            uint32_t ordered = 0, pairs = 0;
            for (uint32_t i = 0; i + step < n; i += step) {
                if (matrix[i * m + 0] <= matrix[(i + step) * m + 0]) ++ordered;
                ++pairs;
            }
            feats_.sortedness_0 = pairs ? static_cast<double>(ordered) / pairs : 1.0;
        }

        // Sampled uniq_x_hat (exact distinct on the sample)
        {
            std::unordered_set<double> seen;
            for (uint32_t i = 0; i < n && seen.size() < S; i += step)
                seen.insert(matrix[i * m + 0]);
            feats_.uniq_x_hat = static_cast<uint32_t>(seen.size());
        }

        if (m >= 2) {
            // Sampled sortedness_1
            {
                uint32_t ordered = 0, pairs = 0;
                for (uint32_t i = 0; i + step < n; i += step) {
                    if (matrix[i * m + 1] <= matrix[(i + step) * m + 1]) ++ordered;
                    ++pairs;
                }
                feats_.sortedness_1 = pairs ? static_cast<double>(ordered) / pairs : 1.0;
            }
            // Sampled uniq_y_hat
            {
                std::unordered_set<double> seen;
                for (uint32_t i = 0; i < n && seen.size() < S; i += step)
                    seen.insert(matrix[i * m + 1]);
                feats_.uniq_y_hat = static_cast<uint32_t>(seen.size());
            }
            feats_.density_product = feats_.uniq_x_hat * feats_.uniq_y_hat;
        }
        return feats_;
    }

    // Phase 3: write U[k] for live strategies (dumpable under GYRO_DEBUG)
    void striate(const GyroOptions& opt, double U[4]) const {
        const auto& f = feats_;
        const double nlog = f.n * std::log2(static_cast<double>(f.n) + 1.0);

        // Rank1D
        U[0] = (f.m <= 1) ? GYRO_C1 * nlog : GYRO_INF;

        // Fenwick2D
        U[1] = GYRO_C1 * nlog + GYRO_C2 * static_cast<double>(f.uniq_y_hat);
        if (!opt.exact) U[1] = GYRO_INF; // not selected when exact not required? keep finite for exact

        // NestedOrProjection
        U[2] = (f.m >= 3) ? U[1] : GYRO_INF;

        // Approx1D
        U[3] = (opt.allow_approx_1d && !opt.exact) ? GYRO_C1 * nlog : GYRO_INF;

        // Memory pressure term (simple)
        if (opt.memory_pressure || (opt.memory_budget_bytes > 0 && f.density_product > 1000000u)) {
            // Prefer nothing extra; Fenwick still chosen under exact
        }
    }

    Strategy gate(const GyroOptions& opt) const {
        double U[4];
        striate(opt, U);

        // argmin under exactness (already encoded in U via INF)
        int best = 0;
        for (int k = 1; k < 4; ++k)
            if (U[k] < U[best]) best = k;

#ifdef GYRO_DEBUG
        std::fprintf(stderr, "[GYRO_DEBUG] n=%u m=%u s0=%.3f s1=%.3f ux=%u uy=%u\n",
                     feats_.n, feats_.m, feats_.sortedness_0, feats_.sortedness_1,
                     feats_.uniq_x_hat, feats_.uniq_y_hat);
        std::fprintf(stderr, "[GYRO_DEBUG] U = [%.1f, %.1f, %.1f, %.1f]  chosen=%d\n",
                     U[0], U[1], U[2], U[3], best);
#endif

        switch (best) {
        case 0: return Strategy::Rank1D;
        case 1: return Strategy::Fenwick2D;
        case 2: return Strategy::NestedOrProjection;
        case 3: return Strategy::Approx1D;
        default: return Strategy::Fenwick2D;
        }
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
    ctrl.observe(matrix_in, n, m, opt.memory_pressure, opt.memory_budget_bytes);
    Strategy strat = ctrl.gate(opt);

    switch (strat) {
    case Strategy::Rank1D:
        rank_1d(matrix_in, n, m, ranks_out);
        if (dom_out) std::fill(dom_out, dom_out + n, 0);
        break;
    case Strategy::Approx1D:
        rank_1d(matrix_in, n, m, ranks_out);
        if (dom_out) std::fill(dom_out, dom_out + n, 0);
        break;
    case Strategy::Fenwick2D:
        exact_rank_2d_fenwick(matrix_in, n, m, ranks_out, dom_out);
        break;
    case Strategy::NestedOrProjection:
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
