#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <geometry/mesh.h>
#include <geometry/point3d.h>
#include <render/ray.h>
#include <render/scene.h>

struct BarycentricCoordinates {
    float lambda1() const { return 1.0f - lambda2 - lambda3; }
    float lambda2{0.0f}, lambda3{0.0f};
};

struct Intersection {
    Point3D point;
    float distance{infinity};
    Normal3D normal;
    BarycentricCoordinates bary;
    uint32_t triangleIndex{0};

    operator bool() const { return std::isfinite(distance); }
};

struct ShadingIntersection : public Intersection {
    ShadingIntersection() = default;
    // allow implicit conversion from basic intersection
    ShadingIntersection(const Intersection& its) : Intersection{its} {}
    const Instance* instance{nullptr};
    OrthonormalSystem shadingFrame;

    Normal3D computeShadingNormal() const;
};

[[nodiscard]] bool intersect(const AABB& aabb, const IntersectionRay& ray);
[[nodiscard]] Intersection intersect(const Triangle& triangle, const IntersectionRay& ray);
[[nodiscard]] Intersection intersect(const Mesh& mesh, const IntersectionRay& ray);
[[nodiscard]] Intersection intersect(const Instance& instance, const IntersectionRay& ray);
/// intersect a ray with the scene and compute shading parameters
[[nodiscard]] ShadingIntersection intersect(const Scene& scene, const IntersectionRay& ray);

#endif // !INTERSECTION_H
