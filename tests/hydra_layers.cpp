// GYR-AXX-001 Track 1 — Hydra receipt for successive-peel kernel
// N = 1 000 000, best-of-3, compare exact_rank_2d_layers vs exact_rank_2d_fenwick.
// Win condition: wall ≤ 0.70× Fenwick on the named class.
// Expectation: lose both H-DOM and H-MEM. Numbers are the receipt for deletion.
// Compile: g++ -O2 -std=c++17 -Iinclude tests/hydra_layers.cpp -o hydra_layers
// Tiny-L is not a win class.

#include "gyro_rank.hpp"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

using namespace gyro;
using Clock = std::chrono::steady_clock;
using ms = std::chrono::duration<double, std::milli>;

static constexpr uint32_t N = 1000000u;
static constexpr int BEST_OF = 3;

// ---------------------------------------------------------------------------
// Generators (named classes)
// ---------------------------------------------------------------------------

// H-DOM: high layer count / domination-heavy.
// Staircase: point i has x = i, y = N-1-i  → every point is its own layer (L ≈ N).
// This is the worst case for successive peeling (O(n·L) ≈ O(n²)).
static void gen_H_DOM(std::vector<double>& mat) {
    mat.resize(static_cast<size_t>(N) * 2);
    for (uint32_t i = 0; i < N; ++i) {
        mat[i * 2 + 0] = static_cast<double>(i);
        mat[i * 2 + 1] = static_cast<double>(N - 1 - i);
    }
}

// H-MEM: memory-pressure class where Fenwick would allocate a large tree.
// All x distinct, all y distinct (random). Fenwick tree size ≈ N.
// Documented budget: FenwickMax + FenwickSum ≈ 2 * (N+4) * sizeof(int32_t) ≈ 8 MB
// plus order + y_rank vectors ≈ another 12 MB. Peeler also spends Θ(n) indices.
// This class tests whether the peeler’s different constant structure can win
// under a hypothetical tight memory budget; in practice both paths use Θ(n).
static void gen_H_MEM(std::vector<double>& mat, uint32_t seed = 42u) {
    mat.resize(static_cast<size_t>(N) * 2);
    uint32_t s = seed;
    for (uint32_t i = 0; i < N; ++i) {
        mat[i * 2 + 0] = lcg_uniform(s) * 1e9;
        mat[i * 2 + 1] = lcg_uniform(s) * 1e9;
    }
}

// ---------------------------------------------------------------------------
// Timing helper (best-of-3)
// ---------------------------------------------------------------------------
template <typename Fn>
static double best_of_3_ms(Fn&& fn) {
    double best = 1e300;
    for (int t = 0; t < BEST_OF; ++t) {
        auto t0 = Clock::now();
        fn();
        auto t1 = Clock::now();
        double dt = ms(t1 - t0).count();
        if (dt < best) best = dt;
    }
    return best;
}

static void run_class(const char* name,
                      const std::vector<double>& mat,
                      bool run_peeler) {
    std::vector<int32_t> ranks_f(N), ranks_l(N);

    // Warm-up Fenwick once
    exact_rank_2d_fenwick(mat.data(), N, 2, ranks_f.data(), nullptr);

    double fenwick_ms = best_of_3_ms([&]() {
        exact_rank_2d_fenwick(mat.data(), N, 2, ranks_f.data(), nullptr);
    });

    double layers_ms = -1.0;
    double ratio = -1.0;
    if (run_peeler) {
        // Warm-up peeler once (may be slow)
        exact_rank_2d_layers(mat.data(), N, 2, ranks_l.data(), nullptr);

        layers_ms = best_of_3_ms([&]() {
            exact_rank_2d_layers(mat.data(), N, 2, ranks_l.data(), nullptr);
        });
        ratio = layers_ms / fenwick_ms;
    }

    std::printf("=== %s  (N=%u, best-of-%d) ===\n", name, N, BEST_OF);
    std::printf("  Fenwick wall : %.1f ms\n", fenwick_ms);
    if (run_peeler) {
        std::printf("  Layers wall  : %.1f ms\n", layers_ms);
        std::printf("  Ratio L/F    : %.3f×\n", ratio);
        std::printf("  Win (≤0.70×) : %s\n", ratio <= 0.70 ? "YES" : "NO");
    } else {
        std::printf("  Layers       : SKIPPED (expected pathological)\n");
    }
    std::printf("\n");
    std::fflush(stdout);
}

int main() {
    std::printf("GYR-AXX-001 Track 1 Hydra — successive peel vs Fenwick\n");
    std::printf("Compile flags intended: g++ -O2 -std=c++17\n");
    std::printf("Win = layers wall ≤ 0.70 × Fenwick wall on the named class.\n");
    std::printf("Tiny-L is not a win class.\n\n");

    std::vector<double> mat;

    // H-DOM: high layer count. Peeler is O(n·L) ≈ O(n²) → will be extremely slow.
    // We still attempt one run; if it exceeds practical time the ratio is ≫ 0.70.
    gen_H_DOM(mat);
    // For H-DOM at N=1e6 the peeler is expected to take minutes–hours.
    // Run Fenwick fully; run peeler only if a safety timeout is not required.
    // Here we run both; the receipt will show the loss.
    run_class("H-DOM (staircase, L≈N)", mat, /*run_peeler=*/true);

    // H-MEM: random distinct coordinates. Fenwick tree ~N entries.
    // Documented Fenwick budget ≈ 2*(N+4)*4 + order/y_rank ≈ 20 MB.
    gen_H_MEM(mat, 42u);
    run_class("H-MEM (random, large Fenwick tree)", mat, /*run_peeler=*/true);

    std::printf("Hydra complete. Expectation: both classes lose (ratio ≫ 0.70).\n");
    std::printf("Receipt for Track 1 honesty deletion.\n");
    return 0;
}
