#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "common.h"
#include "bvh.h"

struct Intersection {
    Point3D intersection_point;
    float t{infinity}; // intersection distance
    Vertex normal;

    operator bool() const { return std::isfinite(t); }
};

bool intersect(const AABB& aabb, const IntersectionRay& ray);
Intersection intersect(const Triangle& triangle, const IntersectionRay& ray);
Intersection intersect(const Mesh& mesh, const IntersectionRay& ray);
Intersection intersect(const Scene& scene, const IntersectionRay& ray);
Intersection intersect(const Mesh& mesh, const IntersectionRay& ray, const std::vector<BVH::Node>& nodes, size_t currentIdx);

#endif // !INTERSECTION_H
