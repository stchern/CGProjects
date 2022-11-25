#ifndef VECTORUTILS_H
#define VECTORUTILS_H

#include "SFML/Graphics.hpp"
#include <cmath>

namespace VectorUtils
{

template <typename T>
T lengthVector2(const sf::Vector2<T>& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

template <typename T>
T dot(const sf::Vector2<T>& lhsV, const sf::Vector2<T>& rhsV) {
    return (lhsV.x * rhsV.x + lhsV.y * rhsV.y);
}

};

#endif // VECTORUTILS_H
