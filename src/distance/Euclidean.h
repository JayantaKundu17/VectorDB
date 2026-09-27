#pragma once

#include <cmath>
#include <stdexcept>

#include "core/Vector.h"

namespace Distance {

inline float euclideanDistance(const Vector& a, const Vector& b) {

    if (a.dimension() != b.dimension()) {
        throw std::invalid_argument(
            "Vectors must have the same dimension"
        );
    }

    float sum = 0.0f;

    for (std::size_t i = 0; i < a.dimension(); ++i) {

        float difference = a[i] - b[i];

        sum += difference * difference;
    }

    return std::sqrt(sum);
}

}