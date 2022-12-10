#include <render/intersection.h>

#include <geometry/mesh.h>
#include <geometry/bvh.h>
#include <render/ray.h>
#include <render/scene.h>

bool intersect(const AABB& aabb, const IntersectionRay& ray)
{
    const Point3D t1 = (aabb.min - ray.origin) * ray.inv_direction;
    const Point3D t2 = (aabb.max - ray.origin) * ray.inv_direction;

    const float tNear = ::min(t1, t2).maxComponent();
    const float tFar = ::max(t1, t2).minComponent();

    return tNear <= tFar && tNear <= ray.tMax && tFar >= ray.tMin;
}

Intersection intersect(const Triangle& triangle, const IntersectionRay& ray)
{
    Intersection result;

    result.normal = cross(triangle.v1v2, triangle.v1v3);
    const float norm = result.normal.norm();
    result.distance =
        dot(triangle.v1 - ray.origin, result.normal) / dot(ray.direction, result.normal);

    if (result.distance > ray.tMin && result.distance < ray.tMax) {
        result.point = ray.origin + ray.direction * result.distance;
        const Vector3D v1p = result.point - triangle.v1;

        /* Compute triangle areas corresponding to barycentric coordinates.
         * The ray is outside if any coordinate is negative
         * or the sum of two coordinates is larger than triangle area.
         * (Then, the third coordinate would be negative.)
         */
        const Vector3D bary2 = cross(v1p, triangle.v1v3);
        if (dot(bary2, result.normal) < 0.0f)
            return {};
        const Vector3D bary3 = cross(triangle.v1v2, v1p);
        if (dot(bary3, result.normal) < 0.0f)
            return {};

        const float invNorm = 1.0f / norm;
        result.bary.lambda2 = bary2.norm() * invNorm;
        result.bary.lambda3 = bary3.norm() * invNorm;
        if (result.bary.lambda1() < 0.0f)
            return {};
        result.normal *= invNorm;

        return result;
    }

    return {};
}

Intersection intersect(const Mesh& mesh, const IntersectionRay& ray)
{
    Intersection result;

    const std::vector<BVH::Node>& nodes = mesh.getBVH().getNodes();
    const std::vector<TriangleIndices>& faces = mesh.getFaces();

    uint32_t currentNodeIndex = 0;
    while (true) {
        const BVH::Node& currentNode = nodes.at(currentNodeIndex);
        // intersected inner node or leaf node
        if (currentNode.facesEnd && intersect(currentNode.bounds, ray)) {
            const uint32_t leftChildIndex = currentNodeIndex * 2 + 1;
            // if the node has children, check them instead (first left, then right)
            if (leftChildIndex < nodes.size() && nodes.at(leftChildIndex).facesEnd) {
                currentNodeIndex = leftChildIndex;

                continue;
            }
            else {
                // leaf node - check all triangles
                auto faceIndices = mesh.getBVH().getFaceIndices(currentNode);
                for (uint32_t i : faceIndices) {
                    const Triangle triangle = mesh.getTriangleFromFace(faces.at(i));
                    const Intersection its = intersect(triangle, ray);

                    if (its.distance < result.distance) {
                        result = its;
                        result.triangleIndex = i;
                    }
                }
            }
        }

        // traverse up after exploring a right child
        while (currentNodeIndex && currentNodeIndex % 2 == 0)
            currentNodeIndex = currentNodeIndex / 2 - 1;

        // if we reach the top again, we are done
        if (!currentNodeIndex)
            break;

        // check right child after each left child
        if (currentNodeIndex % 2 == 1)
            ++currentNodeIndex;
    }

    return result;
}

Intersection intersect(const Instance& instance, const IntersectionRay& ray)
{
    const Point3D newOrigin = instance.toLocal * ray.origin;
    const Vector3D newDir = instance.toLocal.m * ray.direction;

    const Ray localRay{newOrigin, newDir, ray.tMin, ray.tMax};

    Intersection its = intersect(*instance.mesh, localRay);
    if (!its)
        return {};

    its.point = instance.toWorld * its.point;
    its.normal = normalize(instance.normalToWorld * its.normal);

    return its;
}

ShadingIntersection intersect(const Scene& scene, const IntersectionRay& ray)
{
    ShadingIntersection result;

    for (const Instance& instance : scene.getInstances()) {
        Intersection its = intersect(instance, ray);

        if (its.distance < result.distance) {
            result = its;
            result.instance = &instance;
        }
    }

    if (!result)
        return {};

    const Instance& instance = *result.instance;
    const Mesh& mesh = *instance.mesh;

    bool isSmooth = false;

    for (auto [begin, end] : mesh.getSmoothGroups()) {
        if (result.triangleIndex < end) {
            if (result.triangleIndex >= begin)
                isSmooth = true;
            break;
        }
    }

    if (isSmooth)
        result.shadingFrame = {result.computeShadingNormal()};
    else
        result.shadingFrame = {result.normal};

    return result;
}
