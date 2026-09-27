#pragma once

#include <cstddef>
#include "distance/DistanceMetric.h"
#include <random>
#include <vector>

#include "core/SearchResult.h"
#include "core/VectorStore.h"

class HNSW {

public:

    HNSW(
        const VectorStore& store,
        std::size_t M = 32,
        std::size_t efConstruction = 100,
        std::size_t efSearch = 100
    );

    void build();

    std::vector<SearchResult> search(
        const Vector& query,
        std::size_t k,
        DistanceMetric metric
    ) const;

    std::size_t size() const;

private:

    struct Node {

        std::size_t storeIndex;

        std::vector<std::vector<std::size_t>> neighbors;
    };

    const VectorStore& store_;

    std::size_t M_;

    std::size_t efConstruction_;

    std::size_t efSearch_;

    std::vector<Node> nodes_;

    std::size_t entryPoint_;

    int maxLevel_;

    std::mt19937 generator_;

    int randomLevel();

    double distance(
        const Vector& a,
        const Vector& b,
        DistanceMetric metric
    ) const;
};
