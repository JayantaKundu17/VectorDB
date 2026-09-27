#include "indexes/KDTree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include "distance/Cosine.h"
#include "distance/Euclidean.h"
#include "distance/Manhattan.h"


// ============================================================
// Constructor
// ============================================================

KDTree::KDTree(
    const VectorStore& store
) {

    if (store.size() == 0) {

        root = nullptr;
        dimension = 0;

        return;
    }

    const auto& records =
        store.getAll();

    std::vector<VectorRecord> points;

    points.reserve(
        records.size()
    );

    for (const auto& entry : records) {

        points.push_back(
            entry.second
        );
    }

    dimension =
        points[0].vector.dimension();

    root =
        build(
            std::move(points),
            0
        );
}


// ============================================================
// Build KD-Tree
// ============================================================

std::unique_ptr<KDTree::Node>
KDTree::build(
    std::vector<VectorRecord> points,
    std::size_t depth
) {

    if (points.empty()) {
        return nullptr;
    }

    std::size_t axis =
        depth % dimension;

    std::size_t median =
        points.size() / 2;

    std::nth_element(
        points.begin(),
        points.begin() + median,
        points.end(),

        [axis](
            const VectorRecord& a,
            const VectorRecord& b
        ) {

            return a.vector[axis]
                < b.vector[axis];
        }
    );

    VectorRecord medianRecord =
        points[median];

    std::vector<VectorRecord> leftPoints(
        points.begin(),
        points.begin() + median
    );

    std::vector<VectorRecord> rightPoints(
        points.begin() + median + 1,
        points.end()
    );

    auto node =
        std::make_unique<Node>(
            medianRecord,
            axis
        );

    node->left =
        build(
            std::move(leftPoints),
            depth + 1
        );

    node->right =
        build(
            std::move(rightPoints),
            depth + 1
        );

    return node;
}


// ============================================================
// Distance calculation
// ============================================================

float KDTree::calculateDistance(
    const Vector& a,
    const Vector& b,
    DistanceMetric metric
) const {

    switch (metric) {

        case DistanceMetric::COSINE:

            // Convert cosine similarity to distance.
            return 1.0f -
                Distance::cosineSimilarity(
                    a,
                    b
                );

        case DistanceMetric::EUCLIDEAN:

            return Distance::euclideanDistance(
                a,
                b
            );

        case DistanceMetric::MANHATTAN:

            return Distance::manhattanDistance(
                a,
                b
            );
    }

    throw std::invalid_argument(
        "Unknown distance metric"
    );
}


// ============================================================
// Check whether the opposite branch can contain a better point
// ============================================================

bool KDTree::shouldVisitOtherSide(
    const Node* node,
    const Vector& query,
    float currentWorstDistance
) const {

    float difference =
        query[node->axis]
        - node->record.vector[node->axis];

    float axisDistance =
        difference * difference;

    return axisDistance
        <= currentWorstDistance * currentWorstDistance;
}


// ============================================================
// Recursive KD-Tree search
// ============================================================

void KDTree::searchRecursive(
    const Node* node,
    const Vector& query,
    std::size_t k,
    DistanceMetric metric,
    std::vector<SearchResult>& results
) const {

    if (node == nullptr) {
        return;
    }


    // --------------------------------------------------------
    // Calculate distance to current node
    // --------------------------------------------------------

    float distance =
        calculateDistance(
            query,
            node->record.vector,
            metric
        );


    // --------------------------------------------------------
    // Add current point to candidate results
    // --------------------------------------------------------

    results.push_back({
        node->record.id,
        distance
    });


    // --------------------------------------------------------
    // Keep only the K closest candidates
    // --------------------------------------------------------

    std::sort(
        results.begin(),
        results.end(),

        [](
            const SearchResult& a,
            const SearchResult& b
        ) {

            return a.score < b.score;
        }
    );


    if (results.size() > k) {

        results.resize(k);
    }


    // --------------------------------------------------------
    // Determine which branch is closer
    // --------------------------------------------------------

    bool queryGoesLeft =
        query[node->axis]
        < node->record.vector[node->axis];


    const Node* firstBranch =
        queryGoesLeft
        ? node->left.get()
        : node->right.get();


    const Node* secondBranch =
        queryGoesLeft
        ? node->right.get()
        : node->left.get();


    // --------------------------------------------------------
    // Search nearer branch first
    // --------------------------------------------------------

    searchRecursive(
        firstBranch,
        query,
        k,
        metric,
        results
    );


    // --------------------------------------------------------
    // Decide whether the other branch is worth visiting
    // --------------------------------------------------------

    float currentWorstDistance =
        results.empty()
        ? std::numeric_limits<float>::max()
        : results.back().score;


    if (
        results.size() < k
        ||
        shouldVisitOtherSide(
            node,
            query,
            currentWorstDistance
        )
    ) {

        searchRecursive(
            secondBranch,
            query,
            k,
            metric,
            results
        );
    }
}


// ============================================================
// Public search
// ============================================================

std::vector<SearchResult>
KDTree::search(
    const Vector& query,
    std::size_t k,
    DistanceMetric metric
) const {

    std::vector<SearchResult> results;

    if (
        k == 0
        ||
        root == nullptr
    ) {

        return results;
    }


    if (
        query.dimension()
        != dimension
    ) {

        throw std::invalid_argument(
            "Query dimension does not match KD-Tree dimension"
        );
    }


    searchRecursive(
        root.get(),
        query,
        k,
        metric,
        results
    );


    std::sort(
        results.begin(),
        results.end(),

        [](
            const SearchResult& a,
            const SearchResult& b
        ) {

            return a.score < b.score;
        }
    );


    return results;
}