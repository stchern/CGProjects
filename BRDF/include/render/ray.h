#ifndef RAY_H
#define RAY_H

#include <common/constants.h>
#include <geometry/point3d.h>

struct Ray {
    Point3D origin;
    Vector3D direction;

    float tMin{epsilon};
    float tMax{infinity};
};

struct IntersectionRay final : public Ray {
    // component-wise inverse of ray direction for faster intersection tests
    const Vector3D inv_direction{1.0f / direction.x, 1.0f / direction.y, 1.0f / direction.z};
    // allow implicit conversion from basic ray
    IntersectionRay(const Ray& r) : Ray{r} {}
};

#endif // !RAY_H
