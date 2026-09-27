#include <cassert>
#include <cmath>
#include <iostream>

#include "core/VectorStore.h"
#include "indexes/BruteForce.h"
#include "indexes/KDTree.h"

int main() {

    VectorStore store;

    // --------------------------------------------------
    // Test dataset
    // --------------------------------------------------

    store.insert({
        "doc_001",
        Vector({1.0f, 2.0f, 3.0f}),
        "Machine learning"
    });

    store.insert({
        "doc_002",
        Vector({1.1f, 2.1f, 3.1f}),
        "Neural networks"
    });

    store.insert({
        "doc_003",
        Vector({10.0f, 10.0f, 10.0f}),
        "Databases"
    });

    store.insert({
        "doc_004",
        Vector({2.0f, 2.0f, 2.0f}),
        "Algorithms"
    });


    // --------------------------------------------------
    // Build both indexes
    // --------------------------------------------------

    BruteForceIndex bruteForce(store);

    KDTree kdTree(store);


    Vector query({
        1.0f,
        2.0f,
        3.0f
    });


    // --------------------------------------------------
    // KD-Tree vs Brute Force
    // --------------------------------------------------

    auto bruteResults =
        bruteForce.search(
            query,
            3,
            DistanceMetric::EUCLIDEAN
        );

    auto kdResults =
        kdTree.search(
            query,
            3,
            DistanceMetric::EUCLIDEAN
        );


    assert(
        bruteResults.size()
        == kdResults.size()
    );


    // --------------------------------------------------
    // Compare ranking
    // --------------------------------------------------

    for (
        std::size_t i = 0;
        i < bruteResults.size();
        ++i
    ) {

        assert(
            bruteResults[i].id
            ==
            kdResults[i].id
        );
    }


    // --------------------------------------------------
    // Test cosine
    // --------------------------------------------------

    bruteResults =
        bruteForce.search(
            query,
            3,
            DistanceMetric::COSINE
        );

    kdResults =
        kdTree.search(
            query,
            3,
            DistanceMetric::COSINE
        );


    assert(
        bruteResults.size()
        == kdResults.size()
    );


    for (
        std::size_t i = 0;
        i < bruteResults.size();
        ++i
    ) {

        assert(
            bruteResults[i].id
            ==
            kdResults[i].id
        );
    }


    // --------------------------------------------------
    // Test Manhattan
    // --------------------------------------------------

    bruteResults =
        bruteForce.search(
            query,
            3,
            DistanceMetric::MANHATTAN
        );

    kdResults =
        kdTree.search(
            query,
            3,
            DistanceMetric::MANHATTAN
        );


    assert(
        bruteResults.size()
        == kdResults.size()
    );


    for (
        std::size_t i = 0;
        i < bruteResults.size();
        ++i
    ) {

        assert(
            bruteResults[i].id
            ==
            kdResults[i].id
        );
    }


    // --------------------------------------------------
    // Test K = 0
    // --------------------------------------------------

    auto emptyResults =
        kdTree.search(
            query,
            0,
            DistanceMetric::EUCLIDEAN
        );

    assert(
        emptyResults.empty()
    );


    // --------------------------------------------------
    // Success
    // --------------------------------------------------

    std::cout
        << "KDTree tests passed.\n";

    return 0;
}