#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <version>
#if __cpp_lib_math_constants >= 201907L
#include <numbers>
#endif
#include <limits>

#if __cpp_lib_math_constants >= 201907L
static constexpr float pi = std::numbers::pi_v<float>;
#else
static constexpr float pi = 3.14159265359f;
#endif
static constexpr float invPi = 1.0f / pi;
static constexpr float degToRad = pi / 180.0f;
static constexpr float epsilon = 1.0e-4f;
static constexpr float infinity = std::numeric_limits<float>::infinity();

#endif // CONSTANTS_H
