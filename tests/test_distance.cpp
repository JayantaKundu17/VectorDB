#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "core/Vector.h"
#include "distance/Cosine.h"
#include "distance/Euclidean.h"
#include "distance/Manhattan.h"

bool approximatelyEqual(float a, float b, float tolerance = 0.0001f) {
    return std::fabs(a - b) < tolerance;
}

int main() {

    Vector a({
        1.0f,
        2.0f,
        3.0f
    });

    Vector b({
        4.0f,
        5.0f,
        6.0f
    });


    // --------------------------------------------------
    // Cosine similarity
    // --------------------------------------------------

    float cosine =
        Distance::cosineSimilarity(a, b);

    assert(
        approximatelyEqual(
            cosine,
            0.9746318f
        )
    );


    // --------------------------------------------------
    // Euclidean distance
    // --------------------------------------------------

    float euclidean =
        Distance::euclideanDistance(a, b);

    assert(
        approximatelyEqual(
            euclidean,
            5.1961524f
        )
    );


    // --------------------------------------------------
    // Manhattan distance
    // --------------------------------------------------

    float manhattan =
        Distance::manhattanDistance(a, b);

    assert(
        approximatelyEqual(
            manhattan,
            9.0f
        )
    );


    // --------------------------------------------------
    // Dimension mismatch
    // --------------------------------------------------

    Vector differentDimension({
        1.0f,
        2.0f
    });

    bool exceptionThrown = false;

    try {

        Distance::euclideanDistance(
            a,
            differentDimension
        );

    } catch (const std::invalid_argument&) {

        exceptionThrown = true;
    }

    assert(exceptionThrown);


    // --------------------------------------------------
    // Zero-vector cosine test
    // --------------------------------------------------

    Vector zeroVector({
        0.0f,
        0.0f,
        0.0f
    });

    exceptionThrown = false;

    try {

        Distance::cosineSimilarity(
            a,
            zeroVector
        );

    } catch (const std::invalid_argument&) {

        exceptionThrown = true;
    }

    assert(exceptionThrown);


    std::cout << "Distance metric tests passed.\n";

    return 0;
}