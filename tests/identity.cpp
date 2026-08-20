/**
 * Phase 0 identity harness for GyroRank (GYR-CTRL-001)
 *
 * Builds Fenwick / Rank1D reference and compares against the gated
 * execute_gyro_rank path. Bit-identical ranks required for exact paths.
 *
 * Expected on current main (v0.1): FAIL only on ADVERSARIAL fixtures
 * (M==2, sortedness_0 > 0.97, N < 4096, y not monotone) because the
 * controller still auto-escapes to rank_1d. That failure is the Phase 1 ticket.
 *
 * Build:
 *   g++ -O2 -std=c++17 -Iinclude tests/identity.cpp -o identity_test
 *   # optional: place pdqsort.h so __has_include succeeds
 *
 * Exit: non-zero if any identity mismatch (expected today for adversarial).
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

// ---- Deterministic fixture generators (header LCG) ----
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
        mat[i * m + 0] = static_cast<double>(i);           // strictly increasing x
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
        mat[i * m + 1] = static_cast<double>(lcg_next(seed) % 7); // 7 distinct y
        for (uint32_t j = 2; j < m; ++j)
            mat[i * m + j] = lcg_uniform(seed);
    }
}

// Core M==2 check: gated path vs pure Fenwick reference
static void check_m2(const std::string& name, const std::vector<double>& mat, uint32_t n,
                     bool memory_pressure = false) {
    std::vector<int32_t> ranks_gated(n), ranks_ref(n), dom(n);
    exact_rank_2d_fenwick(mat.data(), n, 2, ranks_ref.data(), nullptr);
    execute_gyro_rank(mat.data(), n, 2, ranks_gated.data(), dom.data(), memory_pressure);

    bool ok = ranks_equal(ranks_gated, ranks_ref);
    std::string detail;
    if (!ok) {
        int mism = 0;
        for (uint32_t i = 0; i < n; ++i) if (ranks_gated[i] != ranks_ref[i]) ++mism;
        detail = std::to_string(mism) + "/" + std::to_string(n) + " ranks differ";
    }
    report(name, ok, detail);
}

static void check_m1(const std::string& name, const std::vector<double>& mat, uint32_t n) {
    std::vector<int32_t> ranks_gated(n), ranks_ref(n);
    rank_1d(mat.data(), n, 1, ranks_ref.data());
    execute_gyro_rank(mat.data(), n, 1, ranks_gated.data(), nullptr, false);
    report(name, ranks_equal(ranks_gated, ranks_ref));
}

int main() {
    std::cout << "GyroRank Phase 0 identity harness (GYR-CTRL-001)\n";
    std::cout << "================================================\n";

    const uint32_t Ns[] = {64, 1000, 4095, 4096, 10000};
    uint32_t seed = 42;

    // ---- M=1 fixtures ----
    for (uint32_t n : Ns) {
        std::vector<double> mat;
        fill_random(mat, n, 1, seed);
        check_m1("M1-random-N" + std::to_string(n), mat, n);

        fill_all_equal(mat, n, 1);
        check_m1("M1-all-equal-N" + std::to_string(n), mat, n);

        fill_strict_inc_x(mat, n, 1, seed);
        check_m1("M1-strict-inc-N" + std::to_string(n), mat, n);

        fill_reversed_x(mat, n, 1, seed);
        check_m1("M1-reversed-N" + std::to_string(n), mat, n);
    }

    // ---- M=2 fixtures (should pass on current main except adversarial) ----
    for (uint32_t n : Ns) {
        std::vector<double> mat;
        fill_random(mat, n, 2, seed);
        check_m2("M2-random-N" + std::to_string(n), mat, n);
        check_m2("M2-random-N" + std::to_string(n) + "-mempressure", mat, n, true);

        fill_all_equal(mat, n, 2);
        check_m2("M2-all-equal-N" + std::to_string(n), mat, n);

        fill_strict_inc_x(mat, n, 2, seed);  // high sortedness but N may be >=4096
        check_m2("M2-sorted-x-rand-y-N" + std::to_string(n), mat, n);

        fill_reversed_x(mat, n, 2, seed);
        check_m2("M2-reversed-x-N" + std::to_string(n), mat, n);

        fill_few_unique_y(mat, n, 2, seed);
        check_m2("M2-few-unique-y-N" + std::to_string(n), mat, n);
    }

    // ---- ADVERSARIAL set: N<4096 + sortedness_0 == 1.0 + y random ----
    // Current controller takes Insertion1D → ranks differ from Fenwick.
    // This is the expected Phase 0 failure / Phase 1 ticket.
    {
        const uint32_t adv_ns[] = {64, 1000, 2048, 4095};
        for (uint32_t n : adv_ns) {
            std::vector<double> mat;
            fill_strict_inc_x(mat, n, 2, seed);  // x = 0..n-1 → sortedness_0 = 1.0
            check_m2("ADVERSARIAL-sortedness>0.97-N" + std::to_string(n), mat, n);
            check_m2("ADVERSARIAL-sortedness>0.97-N" + std::to_string(n) + "-mempressure",
                     mat, n, true);
        }
    }

    std::cout << "================================================\n";
    std::cout << "Tests: " << g_tests << "   Failures: " << g_failures << "\n";
    if (g_failures > 0) {
        std::cout << "Phase 0 exit criterion met: harness fails on adversarial fixtures.\n";
        std::cout << "This is expected on current main and is the Phase 1 ticket.\n";
        return 1;
    }
    std::cout << "All identity checks passed.\n";
    return 0;
}
