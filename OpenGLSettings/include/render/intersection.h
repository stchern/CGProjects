#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <geometry/mesh.h>
#include <geometry/point3d.h>
#include <render/ray.h>
#include <render/scene.h>

struct Intersection {
    float distance{infinity};
    BarycentricCoordinates bary{};
    uint32_t triangleIndex{0};
    uint32_t instanceIndex{0};

    operator bool() const { return std::isfinite(distance); }

    Intersection() = default;
    // (implemented in exercise02.cpp)
    static bool intersect(const AABB& aabb, const IntersectionRay& ray);
    // (implemented in exercise02.cpp)
    Intersection(const Triangle& triangle, const IntersectionRay& ray);
    // (implemented in exercise03.cpp)
    Intersection(const Mesh& mesh, const IntersectionRay& ray);
    // (implemented in intersection.cpp)
    /// intersect a ray with the scene
    Intersection(const Scene& scene, const IntersectionRay& ray);
};

struct ShadingIntersection : public Intersection {
    // compute shading parameters for the given intersection
    ShadingIntersection(const Scene& scene, const Intersection& its);
    /// intersect a ray with the scene and compute shading parameters
    ShadingIntersection(const Scene& scene, const IntersectionRay& ray)
        : ShadingIntersection{scene, {scene, ray}}
    {
    }
    Point3D point{};
    OrthonormalSystem shadingFrame;
};

#endif // !INTERSECTION_H
