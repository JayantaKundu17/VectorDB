#include <cassert>
#include <cmath>
#include <iostream>

#include "core/VectorStore.h"
#include "indexes/BruteForce.h"

bool approximatelyEqual(
    float a,
    float b,
    float tolerance = 0.0001f
) {
    return std::fabs(a - b) < tolerance;
}

int main() {

    VectorStore store;

    // --------------------------------------------------
    // Insert test vectors
    // --------------------------------------------------

    store.insert({
        "doc_001",
        Vector({1.0f, 2.0f, 3.0f}),
        "Machine learning document"
    });

    store.insert({
        "doc_002",
        Vector({1.1f, 2.1f, 3.1f}),
        "Neural networks document"
    });

    store.insert({
        "doc_003",
        Vector({10.0f, 10.0f, 10.0f}),
        "Database document"
    });

    store.insert({
        "doc_004",
        Vector({2.0f, 2.0f, 2.0f}),
        "Algorithms document"
    });


    // --------------------------------------------------
    // Create index
    // --------------------------------------------------

    BruteForceIndex index(store);


    // --------------------------------------------------
    // Query
    // --------------------------------------------------

    Vector query({
        1.0f,
        2.0f,
        3.0f
    });


    // --------------------------------------------------
    // Test Cosine Search
    // --------------------------------------------------

    auto cosineResults =
        index.search(
            query,
            3,
            DistanceMetric::COSINE
        );

    assert(cosineResults.size() == 3);

    // Exact query should be the closest
    assert(cosineResults[0].id == "doc_001");

    // doc_002 should be second
    assert(cosineResults[1].id == "doc_002");

    assert(
        approximatelyEqual(
            cosineResults[0].score,
            1.0f
        )
    );


    // --------------------------------------------------
    // Test Euclidean Search
    // --------------------------------------------------

    auto euclideanResults =
        index.search(
            query,
            3,
            DistanceMetric::EUCLIDEAN
        );

    assert(euclideanResults.size() == 3);

    assert(euclideanResults[0].id == "doc_001");

    assert(euclideanResults[1].id == "doc_002");

    assert(
        approximatelyEqual(
            euclideanResults[0].score,
            0.0f
        )
    );


    // --------------------------------------------------
    // Test Manhattan Search
    // --------------------------------------------------

    auto manhattanResults =
        index.search(
            query,
            3,
            DistanceMetric::MANHATTAN
        );

    assert(manhattanResults.size() == 3);

    assert(manhattanResults[0].id == "doc_001");

    assert(manhattanResults[1].id == "doc_002");

    assert(
        approximatelyEqual(
            manhattanResults[0].score,
            0.0f
        )
    );


    // --------------------------------------------------
    // Test K larger than database
    // --------------------------------------------------

    auto allResults =
        index.search(
            query,
            100,
            DistanceMetric::COSINE
        );

    assert(allResults.size() == 4);


    // --------------------------------------------------
    // Test K = 0
    // --------------------------------------------------

    auto emptyResults =
        index.search(
            query,
            0,
            DistanceMetric::COSINE
        );

    assert(emptyResults.empty());


    // --------------------------------------------------
    // Test empty database
    // --------------------------------------------------

    VectorStore emptyStore;

    BruteForceIndex emptyIndex(emptyStore);

    auto noResults =
        emptyIndex.search(
            query,
            5,
            DistanceMetric::COSINE
        );

    assert(noResults.empty());


    std::cout << "BruteForce tests passed.\n";

    return 0;
}