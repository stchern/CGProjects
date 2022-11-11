#include "intersection.h"

#include "mesh.h"
#include "ray.h"
#include "scene.h"

bool intersect(const AABB& aabb, const IntersectionRay& ray)
{
    const Point3D t1 = (aabb.min - ray.origin) * ray.inv_direction;
    const Point3D t2 = (aabb.max - ray.origin) * ray.inv_direction;

    const float tNear = ::min(t1, t2).maxComponent();
    const float tFar = ::max(t1, t2).minComponent();

    return tNear <= tFar && tNear <= ray.t_max && tFar >= ray.t_min;
}

Intersection intersect(const Triangle& triangle, const IntersectionRay& ray)
{
    Intersection result;

    result.normal = cross(triangle.v1v2, triangle.v1v3); // not yet normalized
    result.t = dot(triangle.v1 - ray.origin, result.normal) / dot(ray.direction, result.normal);

    if (result.t > ray.t_min && result.t < ray.t_max) {
        result.intersection_point = ray.origin + ray.direction * result.t;
        const Vector3D v1p = result.intersection_point - triangle.v1;

        // compute un-normalized barycentric coordinates
        // outside if negative or sum larger than triangle area (then the third one would be
        // negative)
        const Vector3D bary2 = cross(v1p, triangle.v1v3);
        if (dot(bary2, result.normal) < 0.0f)
            return {};
        const Vector3D bary3 = cross(triangle.v1v2, v1p);
        if (dot(bary3, result.normal) < 0.0f)
            return {};
        const float norm = result.normal.norm();
        if ((bary2.norm() + bary3.norm()) > norm)
            return {};
        result.normal *= 1.0f / norm;

        return result;
    }

    return {};
}

Intersection intersect(const Scene& scene, const IntersectionRay& ray)
{
    Intersection result;

    for (const Mesh& mesh : scene.getMeshes()) {
        Intersection its = intersect(mesh, ray);

        if (its.t < result.t)
            result = its;
    }

    return result;
}

Intersection intersect(const Mesh& mesh, const IntersectionRay& ray)
{
    Intersection result;

    if (intersect(mesh.getBounds(), ray)) {
        if (mesh.getBVH().isConstructed())  {
            result = intersect(mesh, ray, mesh.getBVH().getNodes(), 0);
        }
        else {
//             brute-force intersection test
            for (const auto& face : mesh.getFaces()) {
                const Triangle triangle = mesh.getTriangleFromFace(face);
                const Intersection its = intersect(triangle, ray);

                if (its.t < result.t)
                    result = its;
            }
        }
    }
    return result;
}


Intersection intersect(const Mesh& mesh, const IntersectionRay& ray, const std::vector<BVH::Node>& nodes, size_t currentIdx)
{
    Intersection result;

    if (!intersect(nodes[currentIdx].bounds, ray))
        return result;

    if (mesh.getBVH().isNodeLeaf(currentIdx)) {
        std::span<const uint32_t> faceIndices = mesh.getBVH().getFaceIndices(nodes[currentIdx]);
        for (const uint32_t faceIdx: faceIndices) {
            const Triangle triangle = mesh.getTriangleFromFaceIndex(faceIdx);
            const Intersection its = intersect(triangle, ray);
            if (its.t < result.t)
                result = its;
        }
        return result;
    }

    Intersection leftResult = intersect(mesh, ray, nodes, currentIdx * 2 + 1);
    Intersection rightResult;
    if (currentIdx * 2 + 2 < nodes.size())
        rightResult = intersect(mesh, ray, nodes, currentIdx * 2 + 2);

    if (leftResult.t < rightResult.t && leftResult.t < result.t)
        return leftResult;
    return rightResult;
}
