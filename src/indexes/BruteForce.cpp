#include "indexes/BruteForce.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "distance/Cosine.h"
#include "distance/Euclidean.h"
#include "distance/Manhattan.h"

BruteForceIndex::BruteForceIndex(
    const VectorStore& store
)
    : store(store) {
}

float BruteForceIndex::calculateScore(
    const Vector& query,
    const Vector& target,
    DistanceMetric metric
) const {

    switch (metric) {

        case DistanceMetric::COSINE:
            return Distance::cosineSimilarity(
                query,
                target
            );

        case DistanceMetric::EUCLIDEAN:
            return Distance::euclideanDistance(
                query,
                target
            );

        case DistanceMetric::MANHATTAN:
            return Distance::manhattanDistance(
                query,
                target
            );
    }

    throw std::invalid_argument(
        "Unknown distance metric"
    );
}

std::vector<SearchResult> BruteForceIndex::search(
    const Vector& query,
    std::size_t k,
    DistanceMetric metric
) const {

    std::vector<SearchResult> results;

    if (k == 0 || store.size() == 0) {
        return results;
    }

    for (const auto& entry : store.getAll()) {

        const VectorRecord& record = entry.second;

        float score = calculateScore(
            query,
            record.vector,
            metric
        );

        results.push_back({
            record.id,
            score
        });
    }

    if (metric == DistanceMetric::COSINE) {

        std::sort(
            results.begin(),
            results.end(),
            [](const SearchResult& a,
               const SearchResult& b) {

                return a.score > b.score;
            }
        );

    } else {

        std::sort(
            results.begin(),
            results.end(),
            [](const SearchResult& a,
               const SearchResult& b) {

                return a.score < b.score;
            }
        );
    }

    if (results.size() > k) {
        results.resize(k);
    }

    return results;
}