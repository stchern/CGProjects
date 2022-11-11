#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "common.h"

struct Intersection {
    Point3D intersection_point;
    float t{infinity}; // intersection distance
    Vertex normal;

    operator bool() const { return std::isfinite(t); }
};

bool intersect(const AABB& aabb, const IntersectionRay& ray);
Intersection intersect(const Triangle& triangle, const IntersectionRay& ray);
// (implemented in exercise03.cpp)
Intersection intersect(const Mesh& mesh, const IntersectionRay& ray);
Intersection intersect(const Scene& scene, const IntersectionRay& ray);

#endif // !INTERSECTION_H
