// Copyright (c) 2026 Heywood Geblomi
// Dual licensed: AGPLv3 (non-commercial) or Commercial License (see LICENSE)

/**
 * @file gyro_rank.hpp
 * @brief GyroRank — Fenwick 2-D weak-dominance ranking
 *
 * Exact O(N log N) 2-objective ranking via FenwickMax + FenwickSum.
 * M <= 1 uses rank_1d. M >= 2 uses the first two objectives.
 * There is no controller. A selector that cannot change ranks is not a part.
 *
 * Build: g++ -O3 -std=c++17 -Iinclude examples/demo.cpp -o demo
 * Optional: place pdqsort.h on the include path.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <utility>

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

inline void execute_gyro_rank(const double* matrix_in,
                              uint32_t n,
                              uint32_t m,
                              int32_t* ranks_out,
                              int32_t* dom_out = nullptr,
                              bool /*memory_pressure*/ = false) {
    if (n == 0 || m == 0) return;
    if (m <= 1) {
        rank_1d(matrix_in, n, m, ranks_out);
        if (dom_out) std::fill(dom_out, dom_out + n, 0);
        return;
    }
    exact_rank_2d_fenwick(matrix_in, n, m, ranks_out, dom_out);
}

} // namespace gyro
