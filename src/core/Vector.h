#pragma once

#include <vector>
#include <cstddef>

class Vector {
private:
    std::vector<float> values;

public:
    Vector() = default;

    explicit Vector(const std::vector<float>& values);

    explicit Vector(std::vector<float>&& values);

    std::size_t dimension() const;

    const std::vector<float>& data() const;

    float operator[](std::size_t index) const;
};