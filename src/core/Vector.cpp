#include "core/Vector.h"

Vector::Vector(const std::vector<float>& values)
    : values(values) {
}

Vector::Vector(std::vector<float>&& values)
    : values(std::move(values)) {
}

std::size_t Vector::dimension() const {
    return values.size();
}

const std::vector<float>& Vector::data() const {
    return values;
}

float Vector::operator[](std::size_t index) const {
    return values.at(index);
}