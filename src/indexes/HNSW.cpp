#include "indexes/HNSW.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

// ============================================================
// Distance functions
// ============================================================

double cosineSimilarity(
    const Vector& a,
    const Vector& b
) {
    const auto& av = a.data();
    const auto& bv = b.data();

    if (av.size() != bv.size()) {
        throw std::invalid_argument(
            "Vector dimensions do not match"
        );
    }

    double dot = 0.0;
    double normA = 0.0;
    double normB = 0.0;

    for (std::size_t i = 0; i < av.size(); ++i) {
        const double x = static_cast<double>(av[i]);
        const double y = static_cast<double>(bv[i]);

        dot += x * y;
        normA += x * x;
        normB += y * y;
    }

    if (normA == 0.0 || normB == 0.0) {
        return 0.0;
    }

    return dot /
           (std::sqrt(normA) * std::sqrt(normB));
}


double euclideanDistance(
    const Vector& a,
    const Vector& b
) {
    const auto& av = a.data();
    const auto& bv = b.data();

    if (av.size() != bv.size()) {
        throw std::invalid_argument(
            "Vector dimensions do not match"
        );
    }

    double sum = 0.0;

    for (std::size_t i = 0; i < av.size(); ++i) {
        const double difference =
            static_cast<double>(av[i]) -
            static_cast<double>(bv[i]);

        sum += difference * difference;
    }

    return std::sqrt(sum);
}


double manhattanDistance(
    const Vector& a,
    const Vector& b
) {
    const auto& av = a.data();
    const auto& bv = b.data();

    if (av.size() != bv.size()) {
        throw std::invalid_argument(
            "Vector dimensions do not match"
        );
    }

    double sum = 0.0;

    for (std::size_t i = 0; i < av.size(); ++i) {
        sum += std::abs(
            static_cast<double>(av[i]) -
            static_cast<double>(bv[i])
        );
    }

    return sum;
}

} // namespace


// ============================================================
// Constructor
// ============================================================

HNSW::HNSW(
    const VectorStore& store,
    std::size_t M,
    std::size_t efConstruction,
    std::size_t efSearch
)
    : store_(store),
      M_(M),
      efConstruction_(efConstruction),
      efSearch_(efSearch),
      entryPoint_(0),
      maxLevel_(-1),
      generator_(42)
{
    if (M_ == 0) {
        throw std::invalid_argument(
            "HNSW M must be greater than zero"
        );
    }

    if (efConstruction_ == 0) {
        throw std::invalid_argument(
            "efConstruction must be greater than zero"
        );
    }

    if (efSearch_ == 0) {
        throw std::invalid_argument(
            "efSearch must be greater than zero"
        );
}
}


// ============================================================
// Distance
// ============================================================

double HNSW::distance(
    const Vector& a,
    const Vector& b,
    DistanceMetric metric
) const {
    switch (metric) {
        case DistanceMetric::COSINE:
            return 1.0 - cosineSimilarity(a, b);

        case DistanceMetric::EUCLIDEAN:
            return euclideanDistance(a, b);

        case DistanceMetric::MANHATTAN:
            return manhattanDistance(a, b);
    }

    throw std::invalid_argument(
        "Unknown distance metric"
    );
}


// ============================================================
// Random level
// ============================================================

int HNSW::randomLevel() {
    std::uniform_real_distribution<double> distribution(
        0.0,
        1.0
    );

    int level = 0;

    constexpr double probability = 0.5;

    while (
        distribution(generator_) < probability &&
        level < 32
    ) {
        ++level;
    }

    return level;
}


// ============================================================
// Build
// ============================================================

void HNSW::build() {

    nodes_.clear();

    entryPoint_ = 0;
    maxLevel_ = -1;

    const auto& records = store_.getAll();

    if (records.empty()) {
        return;
    }

    // --------------------------------------------------------
    // Create deterministic record ordering.
    // --------------------------------------------------------

    std::vector<const VectorRecord*> recordsList;

    recordsList.reserve(records.size());

    for (const auto& pair : records) {
        recordsList.push_back(&pair.second);
    }

    std::sort(
        recordsList.begin(),
        recordsList.end(),
        [](const VectorRecord* a,
           const VectorRecord* b) {
            return a->id < b->id;
        }
    );

    // --------------------------------------------------------
    // Create nodes and assign random levels.
    // --------------------------------------------------------

    nodes_.reserve(recordsList.size());

    for (std::size_t i = 0;
         i < recordsList.size();
         ++i) {

        const int level = randomLevel();

        Node node;

        node.storeIndex = i;

        node.neighbors.resize(
            static_cast<std::size_t>(level + 1)
        );

        nodes_.push_back(
            std::move(node)
        );

        // IMPORTANT:
        // Keep the node with the highest level as
        // the entry point.
        if (level > maxLevel_) {
            maxLevel_ = level;
            entryPoint_ = i;
        }
    }

    if (nodes_.empty()) {
        return;
    }

    // --------------------------------------------------------
    // Helper to prune a neighbor list.
    //
    // Keep the M closest neighbors to the owner node.
    // This is much better than arbitrary resize(M).
    // --------------------------------------------------------

    auto pruneNeighbors =
        [&](std::size_t owner,
            std::size_t level) {

        auto& neighbors =
            nodes_[owner].neighbors[level];

        if (neighbors.size() <= M_) {
            return;
        }

        const Vector& ownerVector =
            recordsList[owner]->vector;

        std::sort(
            neighbors.begin(),
            neighbors.end(),
            [&](std::size_t a,
                std::size_t b) {

                const double da =
                    distance(
                        ownerVector,
                        recordsList[a]->vector,
                        DistanceMetric::EUCLIDEAN
                    );

                const double db =
                    distance(
                        ownerVector,
                        recordsList[b]->vector,
                        DistanceMetric::EUCLIDEAN
                    );

                return da < db;
            }
        );

        neighbors.resize(M_);
    };

    // --------------------------------------------------------
    // Build graph.
    //
    // Each node connects to the closest previous nodes
    // available on each of its levels.
    // --------------------------------------------------------

    for (std::size_t current = 0;
         current < nodes_.size();
         ++current) {

        const Vector& currentVector =
            recordsList[current]->vector;

        for (
            std::size_t level = 0;
            level < nodes_[current].neighbors.size();
            ++level
        ) {

            std::vector<
                std::pair<double, std::size_t>
            > candidates;

            candidates.reserve(current);

            // Find previous nodes that exist on this level.
            // Bound construction work on resource-constrained hosts.
            // Sample older nodes and retain recent candidates.
            const std::size_t candidateLimit =
                std::max<std::size_t>(M_ * 4, 128);

            const std::size_t recentLimit =
                std::min(candidateLimit / 2, current);

            const std::size_t olderCount =
                current - recentLimit;

            const std::size_t globalLimit =
                candidateLimit - recentLimit;

            const std::size_t stride =
                std::max<std::size_t>(
                    1,
                    (olderCount + globalLimit - 1) / globalLimit
                );

            std::vector<std::size_t> previousIndices;
            previousIndices.reserve(candidateLimit);

            for (
                std::size_t previous = 0;
                previous < olderCount;
                previous += stride
            ) {
                previousIndices.push_back(previous);
            }

            for (
                std::size_t previous = olderCount;
                previous < current;
                ++previous
            ) {
                previousIndices.push_back(previous);
            }

            for (std::size_t previous : previousIndices) {

                if (
                    level >=
                    nodes_[previous].neighbors.size()
                ) {
                    continue;
                }

                const double d =
                    distance(
                        currentVector,
                        recordsList[previous]->vector,
                        DistanceMetric::EUCLIDEAN
                    );

                candidates.emplace_back(
                    d,
                    previous
                );
            }

            // Closest first.
            std::sort(
                candidates.begin(),
                candidates.end(),
                [](const auto& a,
                   const auto& b) {
                    return a.first < b.first;
                }
            );

            const std::size_t connectionCount =
                std::min(
                    M_,
                    candidates.size()
                );

            // Add bidirectional connections.
            for (
                std::size_t i = 0;
                i < connectionCount;
                ++i
            ) {

                const std::size_t neighbor =
                    candidates[i].second;

                nodes_[current]
                    .neighbors[level]
                    .push_back(neighbor);

                nodes_[neighbor]
                    .neighbors[level]
                    .push_back(current);

                // Keep reverse edge list controlled.
                pruneNeighbors(
                    neighbor,
                    level
                );
            }

            // Also ensure current's list is bounded.
            pruneNeighbors(
                current,
                level
            );
        }
    }
}


// ============================================================
// Search
// ============================================================

std::vector<SearchResult> HNSW::search(
    const Vector& query,
    std::size_t k,
    DistanceMetric metric
) const {

    if (
        k == 0 ||
        nodes_.empty()
    ) {
        return {};
    }

    const auto& records =
        store_.getAll();

    // --------------------------------------------------------
    // Reconstruct the same deterministic ordering used
    // during build().
    // --------------------------------------------------------

    std::vector<const VectorRecord*> recordsList;

    recordsList.reserve(records.size());

    for (const auto& pair : records) {
        recordsList.push_back(&pair.second);
    }

    std::sort(
        recordsList.begin(),
        recordsList.end(),
        [](const VectorRecord* a,
           const VectorRecord* b) {
            return a->id < b->id;
        }
    );

    if (recordsList.size() != nodes_.size()) {
        throw std::runtime_error(
            "VectorStore changed after HNSW build"
        );
    }

    // --------------------------------------------------------
    // Candidate structure.
    // --------------------------------------------------------

    struct Candidate {
        double distance;
        std::size_t node;
    };

    // Min heap:
    // closest candidate is processed first.
    struct CandidateMinCompare {
        bool operator()(
            const Candidate& a,
            const Candidate& b
        ) const {
            return a.distance > b.distance;
        }
    };

    // Max heap:
    // worst current result is kept at top.
    struct CandidateMaxCompare {
        bool operator()(
            const Candidate& a,
            const Candidate& b
        ) const {
            return a.distance < b.distance;
        }
    };

    // --------------------------------------------------------
    // Start from the highest-level entry point.
    // --------------------------------------------------------

    std::size_t current =
        entryPoint_;

    double currentDistance =
        distance(
            query,
            recordsList[current]->vector,
            metric
        );

    // --------------------------------------------------------
    // Greedy descent through upper layers.
    // --------------------------------------------------------

    for (
        int level = maxLevel_;
        level > 0;
        --level
    ) {

        bool changed = true;

        while (changed) {

            changed = false;

            if (
                static_cast<std::size_t>(level) >=
                nodes_[current].neighbors.size()
            ) {
                break;
            }

            for (
                const std::size_t neighbor :
                nodes_[current].neighbors[level]
            ) {

                const double d =
                    distance(
                        query,
                        recordsList[neighbor]->vector,
                        metric
                    );

                if (d < currentDistance) {

                    current = neighbor;

                    currentDistance = d;

                    changed = true;
                }
            }
        }
    }

    // --------------------------------------------------------
    // Bottom-layer efSearch exploration.
    // --------------------------------------------------------

    std::priority_queue<
        Candidate,
        std::vector<Candidate>,
        CandidateMinCompare
    > candidates;

    std::priority_queue<
        Candidate,
        std::vector<Candidate>,
        CandidateMaxCompare
    > results;

    std::unordered_set<std::size_t> visited;

    candidates.push({
        currentDistance,
        current
    });

    results.push({
        currentDistance,
        current
    });

    visited.insert(current);

    while (!candidates.empty()) {

        const Candidate candidate =
            candidates.top();

        candidates.pop();

        // ----------------------------------------------------
        // Standard HNSW termination condition.
        //
        // Once efSearch candidates have been collected and
        // the closest unexplored candidate is farther away
        // than the worst result, further exploration cannot
        // improve the result set.
        // ----------------------------------------------------

        if (
            results.size() >= efSearch_ &&
            candidate.distance >
                results.top().distance
        ) {
            break;
        }

        if (
            candidate.node >=
            nodes_.size()
        ) {
            continue;
        }

        if (
            nodes_[candidate.node]
                .neighbors.empty()
        ) {
            continue;
        }

        // ----------------------------------------------------
        // Explore level 0.
        // ----------------------------------------------------

        for (
            const std::size_t neighbor :
            nodes_[candidate.node].neighbors[0]
        ) {

            if (
                neighbor >= nodes_.size()
            ) {
                continue;
            }

            if (
                visited.count(neighbor)
            ) {
                continue;
            }

            visited.insert(neighbor);

            const double d =
                distance(
                    query,
                    recordsList[neighbor]->vector,
                    metric
                );

            // ------------------------------------------------
            // Add candidate if result queue has capacity or
            // this point improves the current worst result.
            // ------------------------------------------------

            if (
                results.size() < efSearch_ ||
                d < results.top().distance
            ) {

                candidates.push({
                    d,
                    neighbor
                });

                results.push({
                    d,
                    neighbor
                });

                // Keep only the best efSearch results.
                if (
                    results.size() >
                    efSearch_
                ) {
                    results.pop();
                }
            }
        }
    }

    // --------------------------------------------------------
    // Convert results to sortable vector.
    // --------------------------------------------------------

    std::vector<
        std::pair<double, std::size_t>
    > finalResults;

    while (!results.empty()) {

        finalResults.push_back({
            results.top().distance,
            results.top().node
        });

        results.pop();
    }

    // Closest first.
    std::sort(
        finalResults.begin(),
        finalResults.end(),
        [](const auto& a,
           const auto& b) {

            if (a.first != b.first) {
                return a.first < b.first;
            }

            return a.second < b.second;
        }
    );

    // --------------------------------------------------------
    // Convert to SearchResult.
    // --------------------------------------------------------

    const std::size_t resultCount =
        std::min(
            k,
            finalResults.size()
        );

    std::vector<SearchResult> output;

    output.reserve(resultCount);

    for (
        std::size_t i = 0;
        i < resultCount;
        ++i
    ) {

        const auto& result =
            finalResults[i];

        SearchResult searchResult;

        searchResult.id =
            recordsList[result.second]->id;

        // SearchResult uses "score":
        //
        // COSINE:
        //     distance = 1 - similarity
        //     score    = similarity
        //
        // EUCLIDEAN / MANHATTAN:
        //     smaller distance = better
        //     score = -distance

        if (
            metric ==
            DistanceMetric::COSINE
        ) {

            searchResult.score =
                static_cast<float>(
                    1.0 - result.first
                );

        } else {

            searchResult.score =
                static_cast<float>(
                    -result.first
                );
        }

        output.push_back(
            searchResult
        );
    }

    return output;
}


// ============================================================
// Size
// ============================================================

std::size_t HNSW::size() const {
    return nodes_.size();
}
