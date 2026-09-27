#include <cassert>
#include <iostream>

#include "core/Vector.h"

int main() {

    Vector v({
        1.0f,
        2.0f,
        3.0f
    });

    assert(v.dimension() == 3);

    assert(v[0] == 1.0f);
    assert(v[1] == 2.0f);
    assert(v[2] == 3.0f);

    std::cout << "Vector tests passed.\n";

    return 0;
}