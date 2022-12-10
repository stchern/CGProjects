#ifndef MATRIX3D_H
#define MATRIX3D_H

#ifdef NANOGUI_VERSION
#include <nanogui/vector.h>
#endif

#include "frame.h"
#include "point3d.h"

struct Matrix3D {
    Vector3D x{1.0f, 0.0f, 0.0f};
    Vector3D y{0.0f, 1.0f, 0.0f};
    Vector3D z{0.0f, 0.0f, 1.0f};

    Vector3D operator*(const Vector3D& v) const { return x * v.x + y * v.y + z * v.z; }

    Matrix3D operator*(const Matrix3D& m) const { return {*this * m.x, *this * m.y, *this * m.z}; }

    Matrix3D& operator*=(float scale)
    {
        x *= scale;
        y *= scale;
        z *= scale;

        return *this;
    }

    Matrix3D operator*(float scale) const
    {
        Matrix3D copy{*this};
        copy *= scale;
        return copy;
    }

    Matrix3D& operator/=(float div)
    {
        *this *= 1.0f / div;
        return *this;
    }

    Matrix3D operator/(float div) const
    {
        Matrix3D copy{*this};
        copy /= div;
        return copy;
    }

    static Matrix3D scale(Vector3D v)
    {
        return {{v.x, 0.0f, 0.0f}, {0.0f, v.y, 0.0f}, {0.0f, 0.0f, v.z}};
    }

    [[nodiscard]] Matrix3D transposed() const
    {
        return {
            {x.x, y.x, z.x},
            {x.y, y.y, z.y},
            {x.z, y.z, z.z},
        };
    }

    Vector3D& operator[](uint32_t i)
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

    const Vector3D& operator[](uint32_t i) const
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

    float determinant() const
    {

        return x.x * (y.y * z.z - z.y * y.z) - y.x * (x.y * z.z - z.y * x.z)
             + z.x * (x.y * y.z - y.y * x.z);
    }

    [[nodiscard]] Matrix3D inverse() const
    {
        // based on code by GLM

        return Matrix3D{{y.y * z.z - z.y * y.z, z.y * x.z - x.y * z.z, x.y * y.z - y.y * x.z},
                        {z.x * y.z - y.x * z.z, x.x * z.z - z.x * x.z, y.x * x.z - x.x * y.z},
                        {
                            y.x * z.y - z.x * y.y,
                            z.x * x.y - x.x * z.y,
                            x.x * y.y - y.x * x.y,
                        }}
             / determinant();
    }
};

struct HomogeneousTransformation3D {
    Matrix3D m{};
    Vector3D t{};

    Point3D operator*(const Point3D& p) const { return m * p + t; }

    HomogeneousTransformation3D inverse() const
    {
        Matrix3D inverse = m.inverse();
        return {inverse, -(inverse * t)};
    }

#ifdef NANOGUI_VERSION
    /// convert to nanogui matrix
    operator nanogui::Matrix4f() const
    {
        nanogui::Matrix4f result;

        for (uint32_t i = 0; i < 3; ++i)
            for (uint32_t j = 0; j < 3; ++j)
                result.m[i][j] = m[i][j];

        for (uint32_t j = 0; j < 3; ++j)
            result.m[3][j] = t[j];

        result.m[2][3] = 0.0f;
        result.m[1][3] = 0.0f;
        result.m[0][3] = 0.0f;
        result.m[3][3] = 1.0f;

        return result;
    }
#endif
};

#endif // MATRIX3D_H
