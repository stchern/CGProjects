#ifndef MATRIX4D_H
#define MATRIX4D_H

#ifdef NANOGUI_VERSION
#include <nanogui/vector.h>
#endif

#include "matrix3d.h"

#include <stdexcept>
#include <string>

struct Point4D {
    float x{0.0f}, y{0.0f}, z{0.0f}, w{0.0f};

    /// initialize each member with a different value
    Point4D(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {}
    /// initialize all members with the same value
    Point4D(float xyzw = 0.0f) : Point4D{xyzw, xyzw, xyzw, xyzw} {}
    /// extend a 3D point to 4D (w=0)
    Point4D(const Point3D& p) : x{p.x}, y{p.y}, z{p.z} {}

    bool operator==(const Point4D& other) const = default;

#ifdef NANOGUI_VERSION
    /// conversion from nanogui vector
    Point4D(const nanogui::Vector4f& v) : Point4D{v.x(), v.y(), v.z(), v.w()} {}
    /// conversion to nanogui vector
    operator nanogui::Vector4f() const { return {x, y, z, w}; }
#endif

    Point4D& operator+=(const Point4D& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }
    Point4D operator+(const Point4D& other) const
    {
        Point4D copy{*this};
        copy += other;
        return copy;
    }

    Point4D& operator-=(const Point4D& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }
    Point4D operator-(const Point4D& other) const
    {
        Point4D copy{*this};
        copy -= other;
        return copy;
    }

    Point4D operator-() const
    {
        Point4D copy{*this};
        copy.x = -copy.x;
        copy.y = -copy.y;
        copy.z = -copy.z;
        copy.w = -copy.w;
        return copy;
    }

    /// component-wise multiplication
    Point4D& operator*=(const Point4D& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }
    /// component-wise multiplication
    Point4D operator*(const Point4D& other) const
    {
        Point4D copy{*this};
        copy *= other;
        return copy;
    }

    /// component-wise division
    Point4D& operator/=(const Point4D& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }
    /// component-wise division
    Point4D operator/(const Point4D& other) const
    {
        Point4D copy{*this};
        copy /= other;
        return copy;
    }

    /// scale by a constant
    Point4D& operator*=(float scale)
    {
        x *= scale;
        y *= scale;
        z *= scale;
        w *= scale;
        return *this;
    }
    /// scale by a constant
    Point4D operator*(float scale) const
    {
        Point4D copy{*this};
        copy *= scale;
        return copy;
    }
    /// divide by a constant
    Point4D& operator/=(float div)
    {
        div = 1.0f / div;
        if (std::isinf(div) || std::isnan(div))
            *this = {};
        else
            *this *= div;
        return *this;
    }
    /// divide by a constant
    Point4D operator/(float div) const
    {
        Point4D copy{*this};
        copy /= div;
        return copy;
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
        case 3:
            return w;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 4");
        }
    }

    const float& operator[](uint32_t i) const
    {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 4");
        }
    }
};

/// use the Point4D also for vectors
using Vector4D = Point4D;

struct Matrix4D {
    Vector4D x{1.0f, 0.0f, 0.0f, 0.0f};
    Vector4D y{0.0f, 1.0f, 0.0f, 0.0f};
    Vector4D z{0.0f, 0.0f, 1.0f, 0.0f};
    Vector4D w{0.0f, 0.0f, 0.0f, 1.0f};

    Matrix4D() = default;
    Matrix4D(const Vector4D& x, const Vector4D& y, const Vector4D& z, const Vector4D& w)
        : x{x}, y{y}, z{z}, w{w}
    {
    }

    Matrix4D(const Matrix3D& m) : x{m.x}, y{m.y}, z{m.z} {}
    Matrix4D(const HomogeneousTransformation3D& trafo) : Matrix4D{trafo.m}
    {
        w = {trafo.t.x, trafo.t.y, trafo.t.z, 1.0f};
    }

    Vector4D operator*(const Vector4D& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }

    Matrix4D operator*(const Matrix4D& m) const
    {
        return {*this * m.x, *this * m.y, *this * m.z, *this * m.w};
    }

    Vector4D& operator[](uint32_t i)
    {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 4");
        }
    }

    const Vector4D& operator[](uint32_t i) const
    {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            throw std::range_error("i = " + std::to_string(i) + " must be < 4");
        }
    }

#ifdef NANOGUI_VERSION
    /// convert to nanogui matrix
    operator nanogui::Matrix4f() const
    {
        nanogui::Matrix4f result;

        for (uint32_t i = 0; i < 4; ++i)
            for (uint32_t j = 0; j < 4; ++j)
                result.m[i][j] = (*this)[i][j];

        return result;
    }
    Matrix4D(const nanogui::Matrix4f& m)
    {
        for (uint32_t i = 0; i < 4; ++i)
            for (uint32_t j = 0; j < 4; ++j)
                (*this)[i][j] = m.m[i][j];
    }
#endif
};

/// "to string"
inline std::ostream& operator<<(std::ostream& os, const Point4D& p)
{
    os << '[' << p.x << ", " << p.y << ", " << p.z << ", " << p.w << ']';
    return os;
}

#endif // MATRIX4D_H
