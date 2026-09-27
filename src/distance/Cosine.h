#pragma once

#include <cmath>
#include <stdexcept>

#include "core/Vector.h"

namespace Distance {

inline float cosineSimilarity(const Vector& a, const Vector& b) {

    if (a.dimension() != b.dimension()) {
        throw std::invalid_argument(
            "Vectors must have the same dimension"
        );
    }

    float dotProduct = 0.0f;
    float magnitudeA = 0.0f;
    float magnitudeB = 0.0f;

    for (std::size_t i = 0; i < a.dimension(); ++i) {

        dotProduct += a[i] * b[i];

        magnitudeA += a[i] * a[i];

        magnitudeB += b[i] * b[i];
    }

    magnitudeA = std::sqrt(magnitudeA);
    magnitudeB = std::sqrt(magnitudeB);

    if (magnitudeA == 0.0f || magnitudeB == 0.0f) {
        throw std::invalid_argument(
            "Cosine similarity is undefined for zero vectors"
        );
    }

    return dotProduct / (magnitudeA * magnitudeB);
}

}