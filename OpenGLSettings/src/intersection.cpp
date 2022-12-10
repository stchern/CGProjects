#include <render/intersection.h>

#include <geometry/mesh.h>
#include <render/ray.h>
#include <render/scene.h>

bool Intersection::intersect(const AABB& aabb, const IntersectionRay& ray)
{
    const Point3D t1 = (aabb.min - ray.origin) * ray.inv_direction;
    const Point3D t2 = (aabb.max - ray.origin) * ray.inv_direction;

    const float tNear = ::min(t1, t2).maxComponent();
    const float tFar = ::max(t1, t2).minComponent();

    return tNear <= tFar && tNear <= ray.tMax && tFar >= ray.tMin;
}

Intersection::Intersection(const Triangle& triangle, const IntersectionRay& ray)
{
    Normal3D normal = cross(triangle.v1v2, triangle.v1v3);
    distance = dot(triangle.v1 - ray.origin, normal) / dot(ray.direction, normal);

    if (distance < ray.tMin || distance > ray.tMax) {
        distance = infinity;
        return;
    }

    Point3D point = ray.origin + ray.direction * distance;
    const Vector3D v1p = point - triangle.v1;

    /* Compute triangle areas corresponding to barycentric coordinates.
     * The ray is outside if any coordinate is negative
     * or the sum of two coordinates is larger than triangle area.
     * (Then, the third coordinate would be negative.)
     */
    const Vector3D bary2 = cross(v1p, triangle.v1v3);
    if (dot(bary2, normal) < 0.0f) {
        distance = infinity;
        return;
    }
    const Vector3D bary3 = cross(triangle.v1v2, v1p);
    if (dot(bary3, normal) < 0.0f) {
        distance = infinity;
        return;
    }

    const float invNorm = 1.0f / normal.norm();
    bary.lambda2 = bary2.norm() * invNorm;
    bary.lambda3 = bary3.norm() * invNorm;
    if (bary.lambda1() < 0.0f) {
        distance = infinity;
        return;
    }
}

Intersection::Intersection(const Scene& scene, const IntersectionRay& ray)
{
    for (uint32_t i = 0; i < scene.getInstances().size(); ++i) {
        const Instance& instance = scene.getInstances().at(i);
        const Point3D localOrigin = instance.toLocal * ray.origin;
        const Vector3D localDir = instance.toLocal.m * ray.direction;
        const Ray localRay{localOrigin, localDir, ray.tMin, ray.tMax};
        const Intersection its{instance.mesh, localRay};

        if (its.distance < distance) {
            *this = {its};
            instanceIndex = i;
        }
    }
}

ShadingIntersection::ShadingIntersection(const Scene& scene, const Intersection& its)
    : Intersection{its}
{
    if (!its)
        return;

    const Instance& instance = scene.getInstances().at(its.instanceIndex);

    const auto [p, n] = instance.mesh.computePointAndNormal(triangleIndex, bary);
    point = instance.toWorld * p;
    shadingFrame = normalize(instance.normalToWorld * n);
}
