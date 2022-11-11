#ifndef COMMON_H
#define COMMON_H

#if __cplusplus < 202002L
#error "C++20 or newer is required"
#endif
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#if __cpp_lib_math_constants >= 201907L
#include <numbers>
#endif

#include <cstdint>
#include <limits>
#include <type_traits>

#include "point2d.h"
#include "point3d.h"

// define some type aliases (all of these have x,y,z coordinates)

using Vertex = Point3D;
using Vector3D = Point3D;
using Normal3D = Vector3D;
using TextureCoordinate = Point2D;

// forward declarations

class Mesh;
struct TriangleIndices;
struct AABB;
struct Triangle;

struct Ray;
struct IntersectionRay;

class BVH;

class Integrator;
class Scene;
class Camera;
class Film;

/// round up to the next power of 2
template <typename Int, std::enable_if_t<std::is_integral_v<Int>, int> = 0>
Int roundUpPowerOfTwo(Int x)
{
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --x;
    for (uint32_t i = 1; i < sizeof(x); i <<= 1)
        x |= x >> i;
    ++x;
    return x;
}

#if __cpp_lib_math_constants >= 201907L
static constexpr float pi = std::numbers::pi_v<float>;
#else
static constexpr float pi = 3.14159265359f;
#endif
static constexpr float degToRad = pi / 180.0f;
static constexpr float epsilon = 1.0e-6f;
static constexpr float infinity = std::numeric_limits<float>::infinity();

#endif // COMMON_H
