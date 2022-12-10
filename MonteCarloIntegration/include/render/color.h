#ifndef COLOR_H
#define COLOR_H

#include <geometry/point3d.h>

#ifdef NANOGUI_VERSION
#include <nanogui/vector.h>
#endif

/**
 * @brief Color
 *
 * we define all of its operations in the header file
 * to allow the compiler to easily inline these simple operations
 */
struct Color {
    float r, g, b, a;

    Color() = default;

    /// initialize with an rgba color value
    Color(float r, float g, float b, float a = 1.0f) : r{r}, g{g}, b{b}, a{a} {}
    /// initialize with a constant value
    Color(float rgb) : Color{rgb, rgb, rgb} {}

    bool operator==(const Color& other) const = default;

    /// conversion from Point3D
    explicit Color(const Point3D& v) : Color{v.x, v.y, v.z} {}
    /// conversion to Point3D
    explicit operator Point3D() const { return {r, g, b}; }
#ifdef NANOGUI_VERSION
    /// conversion from nanogui Color
    Color(const nanogui::Color& c) : Color{c.r(), c.g(), c.b(), c.a()} {}
    /// conversion to nanogui Color
    operator nanogui::Color() const { return {r, g, b, a}; }
#endif

    Color& operator+=(const Color& other)
    {
        r += other.r;
        g += other.g;
        b += other.b;
        a += other.a;
        return *this;
    }
    Color operator+(const Color& other) const
    {
        Color copy{*this};
        copy += other;
        return copy;
    }

    Color& operator-=(const Color& other)
    {
        r -= other.r;
        g -= other.g;
        b -= other.b;
        a -= other.a;
        return *this;
    }
    Color operator-(const Color& other) const
    {
        Color copy{*this};
        copy -= other;
        return copy;
    }

    Color operator-() const
    {
        Color copy{*this};
        copy.r = -copy.r;
        copy.g = -copy.g;
        copy.b = -copy.b;
        copy.a = -copy.a;
        return copy;
    }

    /// component-wise multiplication
    Color& operator*=(const Color& other)
    {
        r *= other.r;
        g *= other.g;
        b *= other.b;
        a *= other.a;
        return *this;
    }
    /// component-wise multiplication
    Color operator*(const Color& other) const
    {
        Color copy{*this};
        copy *= other;
        return copy;
    }

    /// component-wise division
    Color& operator/=(const Color& other)
    {
        r /= other.r;
        g /= other.g;
        b /= other.b;
        a /= other.a;
        return *this;
    }
    /// component-wise division
    Color operator/(const Color& other) const
    {
        Color copy{*this};
        copy /= other;
        return copy;
    }

    /// add a constant
    Color& operator+=(float x)
    {
        r += x;
        g += x;
        b += x;
        a += x;
        return *this;
    }
    /// add a constant
    Color operator+(float x) const
    {
        Color copy{*this};
        copy += x;
        return copy;
    }
    /// subtract a constant
    Color& operator-=(float x)
    {
        r -= x;
        g -= x;
        b -= x;
        a -= x;
        return *this;
    }
    /// subtract a constant
    Color operator-(float x) const
    {
        Color copy{*this};
        copy -= x;
        return copy;
    }
    /// scale by a constant
    Color& operator*=(float scale)
    {
        r *= scale;
        g *= scale;
        b *= scale;
        a *= scale;
        return *this;
    }
    /// scale by a constant
    Color operator*(float scale) const
    {
        Color copy{*this};
        copy *= scale;
        return copy;
    }
    /// divide by a constant
    Color& operator/=(float div)
    {
        div = 1.0f / div;
        if (std::isinf(div) || std::isnan(div))
            *this = {};
        else
            *this *= div;
        return *this;
    }
    /// divide by a constant
    Color operator/(float div) const
    {
        Color copy{*this};
        copy /= div;
        return copy;
    }

    float operator[](uint32_t i) const
    {
        switch (i) {
        case 0:
            return r;
        case 1:
            return g;
        case 2:
            return b;
        case 3:
            return a;
        default:
            return 0.0f;
        }
    }

    bool isBlack() const {
        return r == 0.0f && g == 0.0f && b == 0.0f;
    }
};

inline Color sqrt(const Color& c) {
    return {std::sqrt(c.r), std::sqrt(c.g), std::sqrt(c.b), std::sqrt(c.a)};
}

/// "to string"
inline std::ostream& operator<<(std::ostream& os, const Color& c)
{
    os << '[' << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ']';
    return os;
}

#endif // COLOR_H
