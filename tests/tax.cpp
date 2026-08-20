/**
 * Phase 3 tax harness (GYR-CTRL-001 A6)
 * observe+striate wall / Fenwick wall at N=1e6 must be ≤ 0.10.
 *
 * Build: g++ -O2 -std=c++17 -Iinclude tests/tax.cpp -o tax_test && ./tax_test
 */
#include "gyro_rank.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdint>

using namespace gyro;

int main() {
    const uint32_t N = 1000000;
    const uint32_t M = 2;
    std::vector<double> mat(N * M);
    uint32_t seed = 42;
    for (uint32_t i = 0; i < N * M; ++i)
        mat[i] = lcg_uniform(seed);

    std::vector<int32_t> ranks(N);

    // Warm-up Fenwick
    exact_rank_2d_fenwick(mat.data(), N, M, ranks.data(), nullptr);

    // Time pure Fenwick
    auto t0 = std::chrono::high_resolution_clock::now();
    exact_rank_2d_fenwick(mat.data(), N, M, ranks.data(), nullptr);
    auto t1 = std::chrono::high_resolution_clock::now();
    double fenwick_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Time observe + striate (controller only)
    GyroController ctrl;
    GyroOptions opt;
    double U[4];
    t0 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < 20; ++rep) {
        ctrl.observe(mat.data(), N, M, false, 0);
        ctrl.striate(opt, U);
        (void)ctrl.gate(opt);
    }
    t1 = std::chrono::high_resolution_clock::now();
    double obs_ms = std::chrono::duration<double, std::milli>(t1 - t0).count() / 20.0;

    double ratio = (fenwick_ms > 0.0) ? (obs_ms / fenwick_ms) : 0.0;
    std::cout << "Fenwick wall: " << fenwick_ms << " ms\n";
    std::cout << "Observe+striate wall (avg): " << obs_ms << " ms\n";
    std::cout << "Ratio: " << ratio << "\n";

    if (ratio > 0.10) {
        std::cout << "FAIL: tax exceeds 0.10\n";
        return 1;
    }
    std::cout << "PASS: tax ≤ 0.10\n";
    return 0;
}
