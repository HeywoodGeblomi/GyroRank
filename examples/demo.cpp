/**
 * GyroRank demo — generate a random 2-D cloud and rank it
 *
 * Build:
 *   g++ -O3 -std=c++17 -Iinclude examples/demo.cpp -o demo
 *   (optional: place pdqsort.h in the include path)
 */

#include "gyro_rank.hpp"
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    using namespace gyro;

    const uint32_t N = 5000;
    const uint32_t M = 2;

    std::vector<double> mat(N * M);
    uint32_t seed = 42;
    for (uint32_t i = 0; i < N * M; ++i)
        mat[i] = lcg_uniform(seed);

    std::vector<int32_t> ranks(N), dom(N);

    auto t0 = std::chrono::high_resolution_clock::now();
    execute_gyro_rank(mat.data(), N, M, ranks.data(), dom.data(), false);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    int32_t max_rank = 0;
    for (int32_t r : ranks) if (r > max_rank) max_rank = r;

    std::cout << "GyroRank demo\n"
              << "  N = " << N << ", M = " << M << "\n"
              << "  time = " << ms << " ms\n"
              << "  max rank = " << max_rank << "\n"
              << "  first 10 ranks: ";
    for (uint32_t i = 0; i < 10 && i < N; ++i)
        std::cout << ranks[i] << (i + 1 < 10 ? ", " : "\n");

    return 0;
}
