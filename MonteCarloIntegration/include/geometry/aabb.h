#ifndef AABB_H
#define AABB_H

#include "point3d.h"
#include <common/constants.h>

/**
 * @brief Axis Aligned Bounding Box
 */
struct AABB {
    /// component-wise minimum of the positions contained in the bounding box
    Point3D min{infinity};
    /// component-wise maximum of the positions contained in the bounding box
    Point3D max{-infinity};

    /// extend the bounding box to contain the given point
    void extend(const Point3D& p)
    {
        min = ::min(min, p);
        max = ::max(max, p);
    }

    /// returns the size of the bounding box
    Vector3D extents() const { return max - min; }

    /// returns the center of the bounding box
    Point3D center() const { return (min + max) * 0.5f; }

    /// checks whether the point is contained in the bounding box
    bool contains(const Point3D& p) const { return p >= min && p <= max; }

    /// checks whether the given bounding box is contained in this bounding box
    bool contains(const AABB& other) const { return other.min >= min && other.max <= max; }

    /// return the bounding box containing this and the other bounding box
    AABB operator+(const AABB& other) const
    {
        return {::min(min, other.min), ::max(max, other.max)};
    }

    /// extend the bounding box to also contain the other bounding box
    AABB& operator+=(const AABB& other)
    {
        *this = *this + other;
        return *this;
    }

    bool operator==(const AABB& other) const = default;
};

/// "to string"
inline std::ostream& operator<<(std::ostream& os, const AABB& aabb)
{
    os << "AABB[" << aabb.min << ", " << aabb.max << ']';
    return os;
}

#endif // AABB_H
