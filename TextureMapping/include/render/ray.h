#ifndef RAY_H
#define RAY_H

#include <common/constants.h>
#include <geometry/point3d.h>

struct Ray {
    Point3D origin;
    Vector3D direction;

    float tMin{epsilon};
    float tMax{infinity};

    static Ray shadowRay(Point3D origin, Point3D target)
    {
        const Vector3D dist = target - origin;
        float norm = dist.norm();
        return {origin, dist / norm, epsilon, norm * (1.0f - epsilon)};
    }
};

struct IntersectionRay final : public Ray {
    // component-wise inverse of ray direction for faster intersection tests
    const Vector3D inv_direction{direction.inverse()};
    // allow implicit conversion from basic ray
    IntersectionRay(const Ray& r) : Ray{r} {}
};

#endif // !RAY_H
