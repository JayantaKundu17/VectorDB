#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/SearchResult.h"
#include "core/VectorStore.h"
#include "distance/DistanceMetric.h"

class KDTree {

private:

    struct Node {

        VectorRecord record;

        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        std::size_t axis;

        Node(
            const VectorRecord& record,
            std::size_t axis
        )
            : record(record),
              left(nullptr),
              right(nullptr),
              axis(axis) {
        }
    };

    std::unique_ptr<Node> root;

    std::size_t dimension;

    std::unique_ptr<Node> build(
        std::vector<VectorRecord> points,
        std::size_t depth
    );

    void searchRecursive(
        const Node* node,
        const Vector& query,
        std::size_t k,
        DistanceMetric metric,
        std::vector<SearchResult>& results
    ) const;

    float calculateDistance(
        const Vector& a,
        const Vector& b,
        DistanceMetric metric
    ) const;

    bool shouldVisitOtherSide(
        const Node* node,
        const Vector& query,
        float currentWorstDistance
    ) const;

public:

    explicit KDTree(
        const VectorStore& store
    );

    std::vector<SearchResult> search(
        const Vector& query,
        std::size_t k,
        DistanceMetric metric
    ) const;
};