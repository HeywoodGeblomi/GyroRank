/**
 * GYR-AXX-002 Track 2 — published-style Fenwick reference table.
 * No new kernel. Header stays Fenwick-only.
 *
 * Times:
 *   1. exact_rank_2d_fenwick (product path)
 *   2. BNL / nested scan — sample at N=2000 (O(N²); skip 1e5/1e6)
 *   3. Skyline-only sweep (sort-x + one-pass y-best) — labeled inexact for layers
 *
 * Generators (pinned):
 *   random two-col, seed=42
 *   sorted-x random-y
 *   high-L / H-DOM style (many layers)
 *
 * N ∈ {1e5, 1e6}. Best-of-3 wall, g++ -O2 style.
 * BNL matches Fenwick bit-identical on identity suite.
 * Skyline-only is exempt and labeled inexact.
 *
 * Acceptance: R1–R5. promote_ready stays false. Track 3 frozen.
 */

#include "gyro_rank.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstring>

using namespace gyro;
using Clock = std::chrono::high_resolution_clock;

static constexpr int BEST_OF = 3;

static void fill_random(std::vector<double>& mat, uint32_t n, uint32_t& seed) {
    mat.resize(n * 2);
    for (uint32_t i = 0; i < n; ++i) {
        mat[i * 2 + 0] = lcg_uniform(seed);
        mat[i * 2 + 1] = lcg_uniform(seed);
    }
}

static void fill_sorted_x(std::vector<double>& mat, uint32_t n, uint32_t& seed) {
    mat.resize(n * 2);
    for (uint32_t i = 0; i < n; ++i) {
        mat[i * 2 + 0] = static_cast<double>(i) / static_cast<double>(n);
        mat[i * 2 + 1] = lcg_uniform(seed);
    }
}

static void fill_hdom(std::vector<double>& mat, uint32_t n, uint32_t& seed) {
    mat.resize(n * 2);
    const uint32_t k = std::max(1u, static_cast<uint32_t>(std::sqrt(static_cast<double>(n)) + 0.5));
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t chain = i % k;
        uint32_t pos   = i / k;
        mat[i * 2 + 0] = static_cast<double>(pos) + 0.01 * chain + 0.0001 * lcg_uniform(seed);
        mat[i * 2 + 1] = static_cast<double>(pos) + 0.01 * (k - chain) + 0.0001 * lcg_uniform(seed);
    }
}

static void exact_rank_2d_bnl(const double* matrix, uint32_t n, uint32_t m,
                              int32_t* ranks_out) {
    if (n == 0) return;
    std::fill(ranks_out, ranks_out + n, 1);
    if (m < 2) return;
    bool changed = true;
    int guard = 0;
    const int MAX_PASSES = 64;
    while (changed && guard < MAX_PASSES) {
        changed = false;
        ++guard;
        for (uint32_t i = 0; i < n; ++i) {
            double xi = matrix[i * m + 0], yi = matrix[i * m + 1];
            int32_t best = 0;
            for (uint32_t j = 0; j < n; ++j) {
                if (i == j) continue;
                double xj = matrix[j * m + 0], yj = matrix[j * m + 1];
                if (xj <= xi && yj <= yi && (xj < xi || yj < yi)) {
                    if (ranks_out[j] > best) best = ranks_out[j];
                }
            }
            int32_t want = best + 1;
            if (want > ranks_out[i]) {
                ranks_out[i] = want;
                changed = true;
            }
        }
    }
}

static void skyline_only_sweep(const double* matrix, uint32_t n, uint32_t m,
                               int32_t* ranks_out) {
    if (n == 0) return;
    std::fill(ranks_out, ranks_out + n, 2);
    if (m < 2) { std::fill(ranks_out, ranks_out + n, 1); return; }
    std::vector<uint32_t> order(n);
    for (uint32_t i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
        double ax = matrix[a * m + 0], bx = matrix[b * m + 0];
        if (ax != bx) return ax < bx;
        double ay = matrix[a * m + 1], by = matrix[b * m + 1];
        if (ay != by) return ay < by;
        return a < b;
    });
    double min_y = std::numeric_limits<double>::infinity();
    for (uint32_t k = 0; k < n; ++k) {
        uint32_t i = order[k];
        double y = matrix[i * m + 1];
        if (y < min_y) {
            ranks_out[i] = 1;
            min_y = y;
        }
    }
}

static double wall_ms(void (*fn)(const double*, uint32_t, uint32_t, int32_t*),
                      const double* mat, uint32_t n, int32_t* ranks) {
    fn(mat, n, 2, ranks);
    double best = 1e300;
    for (int t = 0; t < BEST_OF; ++t) {
        auto t0 = Clock::now();
        fn(mat, n, 2, ranks);
        auto t1 = Clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (ms < best) best = ms;
    }
    return best;
}

static void fenwick_adapter(const double* mat, uint32_t n, uint32_t /*m*/, int32_t* ranks) {
    exact_rank_2d_fenwick(mat, n, 2, ranks, nullptr);
}

static bool identity_check(uint32_t n) {
    uint32_t seed = 42;
    std::vector<double> mat;
    fill_random(mat, n, seed);
    std::vector<int32_t> r_fen(n), r_bnl(n);
    exact_rank_2d_fenwick(mat.data(), n, 2, r_fen.data(), nullptr);
    exact_rank_2d_bnl(mat.data(), n, 2, r_bnl.data());
    for (uint32_t i = 0; i < n; ++i)
        if (r_fen[i] != r_bnl[i]) return false;
    return true;
}

int main() {
    std::printf("GYR-AXX-002 Track 2 — Fenwick reference table\n");
    std::printf("Host: linux, g++ -O2 style, best-of-%d wall\n", BEST_OF);
    std::printf("Product path: exact_rank_2d_fenwick only\n");
    std::printf("================================================================\n");

    {
        bool ok = identity_check(512) && identity_check(2048);
        std::printf("BNL vs Fenwick identity (N=512,2048): %s\n", ok ? "PASS" : "FAIL");
        if (!ok) {
            std::printf("ABORT: BNL does not match Fenwick.\n");
            return 1;
        }
        uint32_t seed = 42;
        std::vector<double> mat;
        fill_random(mat, 2000, seed);
        std::vector<int32_t> ranks(2000);
        double bnl = wall_ms(exact_rank_2d_bnl, mat.data(), 2000, ranks.data());
        double fen = wall_ms(fenwick_adapter, mat.data(), 2000, ranks.data());
        std::printf("BNL sample N=2000: %.1f ms  (Fenwick %.1f ms, ratio %.1f×)\n",
                    bnl, fen, bnl / (fen > 0 ? fen : 1.0));
        std::printf("BNL at N=1e5/1e6: skipped — O(N²) exceeds practical CI budget.\n");
    }

    struct Gen {
        const char* name;
        void (*fill)(std::vector<double>&, uint32_t, uint32_t&);
    };
    Gen gens[] = {
        {"random (seed=42)", fill_random},
        {"sorted-x random-y", fill_sorted_x},
        {"high-L / H-DOM", fill_hdom},
    };
    const uint32_t sizes[] = {100000u, 1000000u};

    std::printf("\n%-22s %10s %12s %12s %14s\n",
                "Generator", "N", "Fenwick_ms", "BNL_ms", "Skyline_ms");
    std::printf("%-22s %10s %12s %12s %14s\n",
                "----------------------", "----------", "------------", "------------", "--------------");

    for (auto& g : gens) {
        for (uint32_t n : sizes) {
            uint32_t seed = 42;
            std::vector<double> mat;
            g.fill(mat, n, seed);
            std::vector<int32_t> ranks(n);
            double fen = wall_ms(fenwick_adapter, mat.data(), n, ranks.data());
            double sky = wall_ms(skyline_only_sweep, mat.data(), n, ranks.data());
            std::printf("%-22s %10u %12.1f %12s %14.1f\n",
                        g.name, n, fen, "skip O(N²)", sky);
        }
    }

    std::printf("\nNotes:\n");
    std::printf("  Fenwick = exact_rank_2d_fenwick (product path, full layers).\n");
    std::printf("  BNL     = nested rank propagation, full layers, bit-identical to Fenwick.\n");
    std::printf("            O(N²); sample at N=2000; N=1e5/1e6 skipped (practical CI budget).\n");
    std::printf("  Skyline = sort-x + one-pass y-best. Rank-1 membership only.\n");
    std::printf("            Labeled inexact for full layers; does NOT match Fenwick identity.\n");
    std::printf("  N=1e7 not run (job budget).\n");
    std::printf("  No Strategy change. No new kernel. promote_ready=false. Track 3 frozen.\n");
    std::printf("================================================================\n");
    return 0;
}
