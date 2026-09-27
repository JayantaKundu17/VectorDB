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
#include "indexes/HNSW.h"

using Clock = std::chrono::high_resolution_clock;

Vector randomVector(
    std::size_t dimension,
    std::mt19937& generator
) {
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

    std::vector<float> values;
    values.reserve(dimension);

    for (std::size_t i = 0; i < dimension; ++i) {
        values.push_back(distribution(generator));
    }

    return Vector(values);
}

double recallAtK(
    const std::vector<SearchResult>& truth,
    const std::vector<SearchResult>& approximate
) {
    if (truth.empty()) {
        return 0.0;
    }

    std::unordered_set<std::string> truthIds;

    for (const auto& r : truth) {
        truthIds.insert(r.id);
    }

    std::size_t matches = 0;

    for (const auto& r : approximate) {
        if (truthIds.count(r.id)) {
            ++matches;
        }
    }

    return static_cast<double>(matches)
        / static_cast<double>(truth.size());
}

int main() {

    constexpr std::size_t VECTOR_COUNT = 5000;
    constexpr std::size_t DIMENSION = 384;
    constexpr std::size_t K = 10;
    constexpr std::size_t REPETITIONS = 50;

    const std::vector<std::size_t> M_VALUES = {
        8,
        16,
        32
    };

    const std::vector<std::size_t> EF_CONSTRUCTION_VALUES = {
        100,
        200
    };

    const std::vector<std::size_t> EF_SEARCH_VALUES = {
        50,
        100,
        200
    };

    std::mt19937 generator(42);

    std::cout
        << "\n============================================\n"
        << " HNSW Parameter Tuning Benchmark\n"
        << "============================================\n\n";

    std::cout
        << "Vectors       : " << VECTOR_COUNT << '\n'
        << "Dimension     : " << DIMENSION << '\n'
        << "Top-K         : " << K << '\n'
        << "Repetitions   : " << REPETITIONS << "\n\n";

    // --------------------------------------------------------
    // Generate database
    // --------------------------------------------------------

    VectorStore store;

    std::cout << "Generating vectors...\n";

    for (std::size_t i = 0; i < VECTOR_COUNT; ++i) {

        store.insert({
            "vec_" + std::to_string(i),
            randomVector(DIMENSION, generator),
            ""
        });

    }

    // --------------------------------------------------------
    // Generate query
    // --------------------------------------------------------

    Vector query = randomVector(
        DIMENSION,
        generator
    );

    // --------------------------------------------------------
    // Ground truth
    // --------------------------------------------------------

    std::cout
        << "Building brute-force ground truth...\n";

    BruteForceIndex bruteForce(store);

    auto groundTruth = bruteForce.search(
        query,
        K,
        DistanceMetric::EUCLIDEAN
    );

    std::cout << "\n";

    // --------------------------------------------------------
    // Results table
    // --------------------------------------------------------

    std::cout
        << std::left
        << std::setw(6) << "M"
        << std::setw(12) << "efConst"
        << std::setw(10) << "efSearch"
        << std::setw(15) << "Build(ms)"
        << std::setw(15) << "Query(ms)"
        << std::setw(12) << "Recall"
        << std::setw(12) << "Speedup"
        << '\n';

    std::cout
        << "---------------------------------------------------------------\n";

    // --------------------------------------------------------
    // Test combinations
    // --------------------------------------------------------

    for (std::size_t M : M_VALUES) {

        for (
            std::size_t efConstruction :
            EF_CONSTRUCTION_VALUES
        ) {

            for (
                std::size_t efSearch :
                EF_SEARCH_VALUES
            ) {

                std::cout
                    << "Testing M="
                    << M
                    << " efConstruction="
                    << efConstruction
                    << " efSearch="
                    << efSearch
                    << "...\n";

                // Build HNSW

                auto buildStart = Clock::now();

                HNSW index(
                    store,
                    M,
                    efConstruction,
                    efSearch
                );

                index.build();

                auto buildEnd = Clock::now();

                double buildMs =
                    std::chrono::duration<double, std::milli>(
                        buildEnd - buildStart
                    ).count();

                // Query benchmark

                std::vector<SearchResult> results;

                auto queryStart = Clock::now();

                for (
                    std::size_t r = 0;
                    r < REPETITIONS;
                    ++r
                ) {

                    results = index.search(
                        query,
                        K,
                        DistanceMetric::EUCLIDEAN
                    );

                }

                auto queryEnd = Clock::now();

                double queryMs =
                    std::chrono::duration<double, std::milli>(
                        queryEnd - queryStart
                    ).count()
                    / REPETITIONS;

                // Recall

                double recall =
                    recallAtK(
                        groundTruth,
                        results
                    );

                // Speedup

                auto bruteStart = Clock::now();

                for (
                    std::size_t r = 0;
                    r < REPETITIONS;
                    ++r
                ) {

                    bruteForce.search(
                        query,
                        K,
                        DistanceMetric::EUCLIDEAN
                    );

                }

                auto bruteEnd = Clock::now();

                double bruteMs =
                    std::chrono::duration<double, std::milli>(
                        bruteEnd - bruteStart
                    ).count()
                    / REPETITIONS;

                double speedup =
                    bruteMs / queryMs;

                std::cout
                    << std::left
                    << std::setw(6)
                    << M
                    << std::setw(12)
                    << efConstruction
                    << std::setw(10)
                    << efSearch
                    << std::setw(15)
                    << std::fixed
                    << std::setprecision(3)
                    << buildMs
                    << std::setw(15)
                    << queryMs
                    << std::setw(12)
                    << std::setprecision(1)
                    << (recall * 100.0)
                    << std::setw(12)
                    << std::setprecision(3)
                    << speedup
                    << '\n';
            }
        }
    }

    std::cout
        << "\n============================================\n"
        << " Tuning Complete\n"
        << "============================================\n";

    return 0;
}
