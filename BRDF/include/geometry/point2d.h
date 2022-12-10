#pragma once

#include <algorithm> // std::min, std::max
#include <cmath> // std::sqrt
#include <ostream> // std::ostream
#include <stdexcept> // std::range_error

#ifdef NANOGUI_VERSION
#include <nanogui/vector.h>
#endif

/**
 * @brief 2D point
 *
 * we define all of its operations in the header file
 * to allow the compiler to easily inline these simple operations
 */
struct Point2D {
    float x, y;

    /// initialize each member with a different value
    Point2D(float x, float y) : x{x}, y{y} {}
    /// initialize all members with the same value
    Point2D(float xy = 0.0f) : Point2D{xy, xy} {}

    bool operator==(const Point2D& other) const = default;

#ifdef NANOGUI_VERSION
    /// conversion from nanogui vector
    Point2D(const nanogui::Vector2f& v) : Point2D{v.x(), v.y()} {}
    /// conversion to nanogui vector
    operator nanogui::Vector2f() const { return {x, y}; }
#endif

    Point2D& operator+=(const Point2D& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Point2D operator+(const Point2D& other) const
    {
        Point2D copy{*this};
        copy += other;
        return copy;
    }

    Point2D& operator-=(const Point2D& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    Point2D operator-(const Point2D& other) const
    {
        Point2D copy{*this};
        copy -= other;
        return copy;
    }

    Point2D operator-() const
    {
        Point2D copy{*this};
        copy.x = -copy.x;
        copy.y = -copy.y;
        return copy;
    }

    /// component-wise multiplication
    Point2D& operator*=(const Point2D& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    /// component-wise multiplication
    Point2D operator*(const Point2D& other) const
    {
        Point2D copy{*this};
        copy *= other;
        return copy;
    }

    /// component-wise division
    Point2D& operator/=(const Point2D& other)
    {
        x /= other.x;
        y /= other.y;
        return *this;
    }
    /// component-wise division
    Point2D operator/(const Point2D& other) const
    {
        Point2D copy{*this};
        copy /= other;
        return copy;
    }

    /// scale by a constant
    Point2D& operator*=(float scale)
    {
        x *= scale;
        y *= scale;
        return *this;
    }
    /// divide by a constant
    Point2D& operator/=(float div)
    {
        div = 1.0f / div;
        if (std::isinf(div) || std::isnan(div))
            *this = {};
        else
            *this *= div;
        return *this;
    }
    /// divide by a constant
    Point2D operator/(float div) const
    {
        Point2D copy{*this};
        copy /= div;
        return copy;
    }

    /// compute the squared L2 norm (the squared distance to [0.0, 0.0])
    float normSqr() const { return x * x + y * y; }

    /// compute the L2 norm (the distance to [0.0, 0.0])
    float norm() const { return std::sqrt(normSqr()); }

    float minComponent() const { return x < y ? x : y; }

    float maxComponent() const { return x > y ? x : y; }

    float operator[](uint32_t i) const
    {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 2");
        }
    }

    float& operator[](uint32_t i)
    {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 2");
        }
    }
};

/// use the Point2D also for vectors
using Vector2D = Point2D;

/// compute the component-wise minimum
inline Point2D min(const Point2D& a, const Point2D& b)
{
    return {std::min(a.x, b.x), std::min(a.y, b.y)};
}

/// compute the component-wise maximum
inline Point2D max(const Point2D& a, const Point2D& b)
{
    return {std::max(a.x, b.x), std::max(a.y, b.y)};
}

/// compute the component-wise absolute
inline Point2D abs(const Point2D& a) { return ::max(a, -a); }

/// compute the component-wise square root
inline Point2D sqrt(const Point2D& a) { return {std::sqrt(a.x), std::sqrt(a.y)}; }

/// compute the dot product of a and b
inline float dot(const Point2D& a, const Point2D& b) { return a.x * b.x + a.y * b.y; }

/// compute the normalized vector
[[nodiscard]] inline Point2D normalize(const Point2D& p) { return p / p.norm(); }

/// compute the squared distance between a and b
inline float distanceSqr(const Point2D& a, const Point2D& b) { return (a - b).normSqr(); }

/// compute the distance between a and b
inline float distance(const Point2D& a, const Point2D& b) { return (a - b).norm(); }

/// check if all components are less than the other point's components
inline bool operator<(const Point2D& a, const Point2D& b) { return a.x < b.x && a.y < b.y; }

/// check if all components are greater than the other point's components
inline bool operator>(const Point2D& a, const Point2D& b) { return a.x > b.x && a.y > b.y; }

/// check if all components are less than the other point's components or equal
inline bool operator<=(const Point2D& a, const Point2D& b) { return a.x <= b.x && a.y <= b.y; }

/// check if all components are greater than the other point's components or equal
inline bool operator>=(const Point2D& a, const Point2D& b) { return a.x >= b.x && a.y >= b.y; }

/// "to string"
inline std::ostream& operator<<(std::ostream& os, const Point2D& p)
{
    os << '[' << p.x << ", " << p.y << ']';
    return os;
}
