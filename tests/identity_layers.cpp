/**
 * GYR-AXX-001 Track 1 — identity harness for the Layers stub
 *
 * Forces exact_rank_2d_layers against exact_rank_2d_fenwick.
 * Commit 1: the stub currently forwards to Fenwick, so this must be green.
 * Later commits that implement real layers must keep this suite bit-identical.
 *
 * Build:
 *   g++ -O2 -std=c++17 -Iinclude tests/identity_layers.cpp -o identity_layers
 */

#include "gyro_rank.hpp"
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

using namespace gyro;

static int g_failures = 0;
static int g_tests = 0;

static bool ranks_equal(const std::vector<int32_t>& a, const std::vector<int32_t>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) return false;
    return true;
}

static void report(const std::string& name, bool ok, const std::string& detail = "") {
    ++g_tests;
    if (ok) {
        std::cout << "[PASS] " << name << "\n";
    } else {
        ++g_failures;
        std::cout << "[FAIL] " << name;
        if (!detail.empty()) std::cout << " — " << detail;
        std::cout << "\n";
    }
}

static void fill_random(std::vector<double>& mat, uint32_t n, uint32_t m, uint32_t& seed) {
    mat.resize(n * m);
    for (uint32_t i = 0; i < n * m; ++i)
        mat[i] = lcg_uniform(seed);
}

static void fill_all_equal(std::vector<double>& mat, uint32_t n, uint32_t m, double v = 0.5) {
    mat.assign(n * m, v);
}

static void fill_strict_inc_x(std::vector<double>& mat, uint32_t n, uint32_t m, uint32_t& seed) {
    mat.resize(n * m);
    for (uint32_t i = 0; i < n; ++i) {
        mat[i * m + 0] = static_cast<double>(i);
        for (uint32_t j = 1; j < m; ++j)
            mat[i * m + j] = lcg_uniform(seed);
    }
}

static void fill_reversed_x(std::vector<double>& mat, uint32_t n, uint32_t m, uint32_t& seed) {
    mat.resize(n * m);
    for (uint32_t i = 0; i < n; ++i) {
        mat[i * m + 0] = static_cast<double>(n - 1 - i);
        for (uint32_t j = 1; j < m; ++j)
            mat[i * m + j] = lcg_uniform(seed);
    }
}

static void fill_few_unique_y(std::vector<double>& mat, uint32_t n, uint32_t m, uint32_t& seed) {
    mat.resize(n * m);
    for (uint32_t i = 0; i < n; ++i) {
        mat[i * m + 0] = lcg_uniform(seed);
        mat[i * m + 1] = static_cast<double>(lcg_next(seed) % 7);
        for (uint32_t j = 2; j < m; ++j)
            mat[i * m + j] = lcg_uniform(seed);
    }
}

// Core check: layers path vs pure Fenwick reference
static void check_m2(const std::string& name, const std::vector<double>& mat, uint32_t n) {
    std::vector<int32_t> ranks_layers(n), ranks_ref(n);
    exact_rank_2d_fenwick(mat.data(), n, 2, ranks_ref.data(), nullptr);
    exact_rank_2d_layers(mat.data(), n, 2, ranks_layers.data(), nullptr);

    bool ok = ranks_equal(ranks_layers, ranks_ref);
    std::string detail;
    if (!ok) {
        int mism = 0;
        for (uint32_t i = 0; i < n; ++i) if (ranks_layers[i] != ranks_ref[i]) ++mism;
        detail = std::to_string(mism) + "/" + std::to_string(n) + " ranks differ";
    }
    report(name, ok, detail);
}

int main() {
    std::cout << "GYR-AXX-001 Track 1 — Layers identity vs Fenwick\n";
    std::cout << "================================================\n";

    const uint32_t Ns[] = {64, 1000, 4095, 4096, 10000};
    uint32_t seed = 42;

    for (uint32_t n : Ns) {
        std::vector<double> mat;
        fill_random(mat, n, 2, seed);
        check_m2("M2-random-N" + std::to_string(n), mat, n);

        fill_all_equal(mat, n, 2);
        check_m2("M2-all-equal-N" + std::to_string(n), mat, n);

        fill_strict_inc_x(mat, n, 2, seed);
        check_m2("M2-sorted-x-rand-y-N" + std::to_string(n), mat, n);

        fill_reversed_x(mat, n, 2, seed);
        check_m2("M2-reversed-x-N" + std::to_string(n), mat, n);

        fill_few_unique_y(mat, n, 2, seed);
        check_m2("M2-few-unique-y-N" + std::to_string(n), mat, n);
    }

    // Adversarial high-sortedness cases (must still match Fenwick)
    {
        const uint32_t adv_ns[] = {64, 1000, 2048, 4095};
        for (uint32_t n : adv_ns) {
            std::vector<double> mat;
            fill_strict_inc_x(mat, n, 2, seed);
            check_m2("ADVERSARIAL-sorted-x-N" + std::to_string(n), mat, n);
        }
    }

    std::cout << "================================================\n";
    std::cout << "Tests: " << g_tests << "   Failures: " << g_failures << "\n";
    if (g_failures > 0) {
        std::cout << "Identity failed — layers stub is not bit-identical to Fenwick.\n";
        return 1;
    }
    std::cout << "All layers-vs-Fenwick identity checks passed (Commit 1 stub is green).\n";
    return 0;
}
