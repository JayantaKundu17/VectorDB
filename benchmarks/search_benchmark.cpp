#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/VectorStore.h"
#include "indexes/BruteForce.h"
#include "indexes/KDTree.h"
#include "indexes/HNSW.h"

using Clock = std::chrono::high_resolution_clock;


// ============================================================
// Generate random vector
// ============================================================

Vector randomVector(
    std::size_t dimension,
    std::mt19937& generator
) {
    std::uniform_real_distribution<float> distribution(
        -1.0f,
        1.0f
    );

    std::vector<float> values;
    values.reserve(dimension);

    for (std::size_t i = 0; i < dimension; ++i) {
        values.push_back(
            distribution(generator)
        );
    }

    return Vector(values);
}


// ============================================================
// Benchmark result
// ============================================================

struct BenchmarkResult {
    double milliseconds;
    std::size_t resultCount;
};


// ============================================================
// Benchmark Brute Force
// ============================================================

BenchmarkResult benchmarkBruteForce(
    const BruteForceIndex& index,
    const Vector& query,
    std::size_t k,
    std::size_t repetitions
) {
    std::size_t resultCount = 0;

    auto start = Clock::now();

    for (std::size_t i = 0; i < repetitions; ++i) {

        auto results =
            index.search(
                query,
                k,
                DistanceMetric::EUCLIDEAN
            );

        resultCount = results.size();
    }

    auto end = Clock::now();

    double milliseconds =
        std::chrono::duration<double, std::milli>(
            end - start
        ).count();

    return {
        milliseconds / repetitions,
        resultCount
    };
}


// ============================================================
// Benchmark KD-Tree
// ============================================================

BenchmarkResult benchmarkKDTree(
    const KDTree& index,
    const Vector& query,
    std::size_t k,
    std::size_t repetitions
) {
    std::size_t resultCount = 0;

    auto start = Clock::now();

    for (std::size_t i = 0; i < repetitions; ++i) {

        auto results =
            index.search(
                query,
                k,
                DistanceMetric::EUCLIDEAN
            );

        resultCount = results.size();
    }

    auto end = Clock::now();

    double milliseconds =
        std::chrono::duration<double, std::milli>(
            end - start
        ).count();

    return {
        milliseconds / repetitions,
        resultCount
    };
}


// ============================================================
// Benchmark HNSW
// ============================================================

BenchmarkResult benchmarkHNSW(
    const HNSW& index,
    const Vector& query,
    std::size_t k,
    std::size_t repetitions
) {
    std::size_t resultCount = 0;

    auto start = Clock::now();

    for (std::size_t i = 0; i < repetitions; ++i) {

        auto results =
            index.search(
                query,
                k,
                DistanceMetric::EUCLIDEAN
            );

        resultCount = results.size();
    }

    auto end = Clock::now();

    double milliseconds =
        std::chrono::duration<double, std::milli>(
            end - start
        ).count();

    return {
        milliseconds / repetitions,
        resultCount
    };
}


// ============================================================
// Recall@K
// ============================================================

double calculateRecall(
    const std::vector<SearchResult>& groundTruth,
    const std::vector<SearchResult>& approximate
) {
    if (groundTruth.empty()) {
        return 0.0;
    }

    std::unordered_set<std::string> truthIds;

    for (const auto& result : groundTruth) {
        truthIds.insert(result.id);
    }

    std::size_t matches = 0;

    for (const auto& result : approximate) {
        if (truthIds.count(result.id) > 0) {
            ++matches;
        }
    }

    return static_cast<double>(matches)
        / static_cast<double>(groundTruth.size());
}


// ============================================================
// Main benchmark
// ============================================================

int main() {

    constexpr std::size_t VECTOR_COUNT =
        5000;

    constexpr std::size_t K =
        10;

    constexpr std::size_t REPETITIONS =
        100;

    std::vector<std::size_t> dimensions = {
        32,
        128,
        384,
        768
    };

    std::mt19937 generator(42);


    std::cout
        << "\n============================================\n"
        << " VectorDB Search Benchmark\n"
        << "============================================\n\n";

    std::cout
        << "Vectors      : "
        << VECTOR_COUNT
        << '\n';

    std::cout
        << "Top-K        : "
        << K
        << '\n';

    std::cout
        << "Repetitions  : "
        << REPETITIONS
        << "\n\n";


    // ========================================================
    // Test each dimension
    // ========================================================

    for (std::size_t dimension : dimensions) {

        std::cout
            << "--------------------------------------------\n";

        std::cout
            << "Dimension: "
            << dimension
            << '\n';

        std::cout
            << "Generating vectors...\n";


        // ----------------------------------------------------
        // Create database
        // ----------------------------------------------------

        VectorStore store;


        // ----------------------------------------------------
        // Generate random vectors
        // ----------------------------------------------------

        for (
            std::size_t i = 0;
            i < VECTOR_COUNT;
            ++i
        ) {

            store.insert({
                "vec_" +
                std::to_string(i),

                randomVector(
                    dimension,
                    generator
                )
            });
        }


        // ----------------------------------------------------
        // Build Brute Force index
        // ----------------------------------------------------

        std::cout
            << "Building Brute Force index...\n";

        BruteForceIndex bruteForce(store);


        // ----------------------------------------------------
        // Build KD-Tree
        // ----------------------------------------------------

        std::cout
            << "Building KD-Tree...\n";

        auto kdStart = Clock::now();

        KDTree kdTree(store);

        auto kdEnd = Clock::now();

        double kdBuildTime =
            std::chrono::duration<double, std::milli>(
                kdEnd - kdStart
            ).count();


        // ----------------------------------------------------
        // Build HNSW
        // ----------------------------------------------------

        std::cout
            << "Building HNSW...\n";

        auto hnswStart = Clock::now();

        HNSW hnsw(
            store,
            32,
            100,
            100
        );

        hnsw.build();

        auto hnswEnd = Clock::now();

        double hnswBuildTime =
            std::chrono::duration<double, std::milli>(
                hnswEnd - hnswStart
            ).count();


        // ----------------------------------------------------
        // Generate query
        // ----------------------------------------------------

        Vector query =
            randomVector(
                dimension,
                generator
            );


        // ----------------------------------------------------
        // Ground truth
        // ----------------------------------------------------

        auto groundTruth =
            bruteForce.search(
                query,
                K,
                DistanceMetric::EUCLIDEAN
            );


        // ----------------------------------------------------
        // Benchmark
        // ----------------------------------------------------

        auto bruteResult =
            benchmarkBruteForce(
                bruteForce,
                query,
                K,
                REPETITIONS
            );

        auto kdResult =
            benchmarkKDTree(
                kdTree,
                query,
                K,
                REPETITIONS
            );

        auto hnswResult =
            benchmarkHNSW(
                hnsw,
                query,
                K,
                REPETITIONS
            );


        // ----------------------------------------------------
        // Get approximate results once
        // ----------------------------------------------------

        auto kdResults =
            kdTree.search(
                query,
                K,
                DistanceMetric::EUCLIDEAN
            );

        auto hnswResults =
            hnsw.search(
                query,
                K,
                DistanceMetric::EUCLIDEAN
            );


        double kdRecall =
            calculateRecall(
                groundTruth,
                kdResults
            );

        double hnswRecall =
            calculateRecall(
                groundTruth,
                hnswResults
            );


        // ----------------------------------------------------
        // Results
        // ----------------------------------------------------

        std::cout
            << "\nResults:\n\n";


        std::cout
            << "Brute Force\n";

        std::cout
            << "  Query latency : "
            << std::fixed
            << std::setprecision(4)
            << bruteResult.milliseconds
            << " ms\n";

        std::cout
            << "  Results       : "
            << bruteResult.resultCount
            << "\n";


        std::cout
            << "\nKD-Tree\n";

        std::cout
            << "  Build time    : "
            << kdBuildTime
            << " ms\n";

        std::cout
            << "  Query latency : "
            << kdResult.milliseconds
            << " ms\n";

        std::cout
            << "  Results       : "
            << kdResult.resultCount
            << "\n";

        std::cout
            << "  Recall@10     : "
            << kdRecall * 100.0
            << "%\n";


        std::cout
            << "\nHNSW\n";

        std::cout
            << "  Build time    : "
            << hnswBuildTime
            << " ms\n";

        std::cout
            << "  Query latency : "
            << hnswResult.milliseconds
            << " ms\n";

        std::cout
            << "  Results       : "
            << hnswResult.resultCount
            << "\n";

        std::cout
            << "  Recall@10     : "
            << hnswRecall * 100.0
            << "%\n";


        // ----------------------------------------------------
        // Speedups
        // ----------------------------------------------------

        double kdSpeedup =
            bruteResult.milliseconds /
            kdResult.milliseconds;

        double hnswSpeedup =
            bruteResult.milliseconds /
            hnswResult.milliseconds;


        std::cout
            << "\nKD-Tree Speedup: "
            << kdSpeedup
            << "x\n";

        std::cout
            << "HNSW Speedup:    "
            << hnswSpeedup
            << "x\n\n";
    }


    std::cout
        << "============================================\n"
        << " Benchmark complete\n"
        << "============================================\n";

    return 0;
}
