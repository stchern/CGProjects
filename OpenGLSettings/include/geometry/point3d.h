#pragma once

#include <algorithm> // std::min, std::max
#include <cmath> // std::sqrt
#include <ostream> // std::ostream
#include <stdexcept> // std::range_error
#include <string> // std::to_string

#ifdef NANOGUI_VERSION
#include <nanogui/vector.h>
#endif

/**
 * @brief 3D point
 *
 * we define all of its operations in the header file
 * to allow the compiler to easily inline these simple operations
 */
struct Point3D {
    float x, y, z;

    /// initialize each member with a different value
    Point3D(float x, float y, float z) : x{x}, y{y}, z{z} {}
    /// initialize all members with the same value
    Point3D(float xyz = 0.0f) : Point3D{xyz, xyz, xyz} {}

    bool operator==(const Point3D& other) const = default;

#ifdef NANOGUI_VERSION
    /// conversion from nanogui vector
    Point3D(const nanogui::Vector3f& v) : Point3D{v.x(), v.y(), v.z()} {}
    /// conversion to nanogui vector
    operator nanogui::Vector3f() const { return {x, y, z}; }
#endif

    Point3D& operator+=(const Point3D& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    Point3D operator+(const Point3D& other) const
    {
        Point3D copy{*this};
        copy += other;
        return copy;
    }

    Point3D& operator-=(const Point3D& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    Point3D operator-(const Point3D& other) const
    {
        Point3D copy{*this};
        copy -= other;
        return copy;
    }

    Point3D operator-() const
    {
        Point3D copy{*this};
        copy.x = -copy.x;
        copy.y = -copy.y;
        copy.z = -copy.z;
        return copy;
    }

    /// component-wise multiplication
    Point3D& operator*=(const Point3D& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }
    /// component-wise multiplication
    Point3D operator*(const Point3D& other) const
    {
        Point3D copy{*this};
        copy *= other;
        return copy;
    }

    /// component-wise division
    Point3D& operator/=(const Point3D& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        return *this;
    }
    /// component-wise division
    Point3D operator/(const Point3D& other) const
    {
        Point3D copy{*this};
        copy /= other;
        return copy;
    }

    /// scale by a constant
    Point3D& operator*=(float scale)
    {
        x *= scale;
        y *= scale;
        z *= scale;
        return *this;
    }
    /// scale by a constant
    Point3D operator*(float scale) const
    {
        Point3D copy{*this};
        copy *= scale;
        return copy;
    }
    /// divide by a constant
    Point3D& operator/=(float div)
    {
        div = 1.0f / div;
        if (std::isinf(div) || std::isnan(div))
            *this = {};
        else
            *this *= div;
        return *this;
    }
    /// divide by a constant
    Point3D operator/(float div) const
    {
        Point3D copy{*this};
        copy /= div;
        return copy;
    }

    /// compute the component-wise inverse
    Point3D inverse() const { return {1.0f / x, 1.0f / y, 1.0f / z}; }

    /// compute the squared L2 norm (the squared distance to [0.0, 0.0, 0.0])
    float normSqr() const { return x * x + y * y + z * z; }

    /// compute the L2 norm (the distance to [0.0, 0.0, 0.0])
    float norm() const { return std::sqrt(normSqr()); }

    /// returns the minimum component
    float minComponent() const { return x < y ? (x < z ? x : z) : (y < z ? y : z); }
    /// returns the maximum component
    float maxComponent() const { return x > y ? (x > z ? x : z) : (y > z ? y : z); }

    /// returns the index of the minimum component
    uint8_t minDimension() const { return x < y ? (x < z ? 0 : 2) : (y < z ? 1 : 2); }
    /// returns the index of the maximum component
    uint8_t maxDimension() const { return x > y ? (x > z ? 0 : 2) : (y > z ? 1 : 2); }

    float operator[](uint32_t i) const
    {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 3");
        }
    }

    float& operator[](uint32_t i)
    {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 3");
        }
    }
};

/// use the Point3D also for vectors
using Vector3D = Point3D;
/// use the Point3D also for normals
using Normal3D = Vector3D;

/// compute the component-wise minimum
inline Point3D min(const Point3D& a, const Point3D& b)
{
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

/// compute the component-wise maximum
inline Point3D max(const Point3D& a, const Point3D& b)
{
    return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

/// compute the component-wise absolute
inline Point3D abs(const Point3D& a) { return ::max(a, -a); }

/// compute the component-wise square root
inline Point3D sqrt(const Point3D& a) { return {std::sqrt(a.x), std::sqrt(a.y), std::sqrt(a.z)}; }

/// compute the dot product of a and b
inline float dot(const Point3D& a, const Point3D& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

/// compute the cross product of a and b
inline Point3D cross(const Point3D& a, const Point3D& b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

/// compute the normalized vector
[[nodiscard]] inline Point3D normalize(const Point3D& p) { return p / p.norm(); }

/// compute the squared distance between a and b
inline float distanceSqr(const Point3D& a, const Point3D& b) { return (a - b).normSqr(); }

/// compute the distance between a and b
inline float distance(const Point3D& a, const Point3D& b) { return (a - b).norm(); }

/// check if all components are less than the other point's components
inline bool operator<(const Point3D& a, const Point3D& b)
{
    return a.x < b.x && a.y < b.y && a.z < b.z;
}

/// check if all components are greater than the other point's components
inline bool operator>(const Point3D& a, const Point3D& b)
{
    return a.x > b.x && a.y > b.y && a.z > b.z;
}

/// check if all components are less than the other point's components or equal
inline bool operator<=(const Point3D& a, const Point3D& b)
{
    return a.x <= b.x && a.y <= b.y && a.z <= b.z;
}

/// check if all components are greater than the other point's components or equal
inline bool operator>=(const Point3D& a, const Point3D& b)
{
    return a.x >= b.x && a.y >= b.y && a.z >= b.z;
}

/// "to string"
inline std::ostream& operator<<(std::ostream& os, const Point3D& p)
{
    os << '[' << p.x << ", " << p.y << ", " << p.z << ']';
    return os;
}
