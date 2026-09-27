#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/SearchResult.h"
#include "core/VectorStore.h"
#include "distance/DistanceMetric.h"

class BruteForceIndex {

private:

    const VectorStore& store;

    float calculateScore(
        const Vector& query,
        const Vector& target,
        DistanceMetric metric
    ) const;

public:

    explicit BruteForceIndex(
        const VectorStore& store
    );

    std::vector<SearchResult> search(
        const Vector& query,
        std::size_t k,
        DistanceMetric metric
    ) const;
};