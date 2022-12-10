#include <geometry/bvh.h>
#include <geometry/mesh.h>
#include <render/intersection.h>
#include <render/ray.h>

#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>

void BVH::construct(const Mesh& mesh)
{
    *this = {}; // clear all previous data

    if (mesh.getFaces().size() > std::numeric_limits<uint32_t>::max()) {
        std::cerr << "mesh contains too many faces for 32bit indices" << std::endl;
        return;
    }

    const uint32_t numFaces = static_cast<uint32_t>(mesh.getFaces().size());

    std::cout << "Building a BVH for a mesh containing " << numFaces << " faces... " << std::flush;

    // estimate the number of BVH nodes and reserve space
    {
        uint32_t numNodes = std::bit_ceil(numFaces / numFacesPerLeaf);
        numNodes = std::min(numNodes, (1U << maxDepth)) - 1;

        nodes.reserve(numNodes);
        faceIndices.reserve(numFaces);
    }

    // initialize the BVH
    nodes.push_back(Node{mesh.getBounds(), 0, numFaces});
    for (uint32_t i = 0; i < numFaces; ++i)
        faceIndices.push_back(i);

    // compute the center of the triangle behind a given index
    auto triangleCenter = [&](uint32_t triangleIndex, uint8_t dim) -> float {
        const Triangle triangle = mesh.getTriangleFromFaceIndex(triangleIndex);
        return (triangle.v1[dim] + triangle.v2[dim] + triangle.v3[dim]) * (1.0f / 3.0f);
    };

    for (uint32_t i = 0; i < nodes.size() && i < (nodes.capacity() - 1) / 2; ++i) {
        const Node& currentNode = nodes.at(i);
        if (getFaceIndices(currentNode).size() <= numFacesPerLeaf)
            continue;

        const uint8_t splitDim = currentNode.bounds.extents().maxDimension();
        auto currentFaces = getFaceIndices(currentNode);

        // median split
        auto center = currentFaces.begin() + currentFaces.size() / 2;
        std::nth_element(currentFaces.begin(), center, currentFaces.end(),
                         [&](uint32_t a, uint32_t b) -> bool {
                             return triangleCenter(a, splitDim) < triangleCenter(b, splitDim);
                         });

        Node left;
        Node right;

        left.facesBegin = currentNode.facesBegin;
        left.facesEnd = right.facesBegin =
            currentNode.facesBegin
            + static_cast<uint32_t>(std::distance(currentFaces.begin(), center));
        right.facesEnd = currentNode.facesEnd;

        for (uint32_t faceIndex : getFaceIndices(left)) {
            const Triangle triangle = mesh.getTriangleFromFaceIndex(faceIndex);
            left.bounds.extend(triangle.v1);
            left.bounds.extend(triangle.v2);
            left.bounds.extend(triangle.v3);
        }
        for (uint32_t faceIndex : getFaceIndices(right)) {
            const Triangle triangle = mesh.getTriangleFromFaceIndex(faceIndex);
            right.bounds.extend(triangle.v1);
            right.bounds.extend(triangle.v2);
            right.bounds.extend(triangle.v3);
        }

        nodes.resize(2 * i + 1); // in case some nodes were skipped
        nodes.push_back(left); // this will be at 2*i+1
        nodes.push_back(right); // this will be at 2*i+2
    }

    std::cout << "done. (used " << nodes.size() << " BVH nodes)" << std::endl;
}

Intersection::Intersection(const Mesh& mesh, const IntersectionRay& ray)
{
    const std::vector<BVH::Node>& nodes = mesh.getBVH().getNodes();
    const std::vector<TriangleIndices>& faces = mesh.getFaces();

    uint32_t currentNodeIndex = 0;
    while (true) {
        const BVH::Node& currentNode = nodes.at(currentNodeIndex);
        // intersected inner node or leaf node
        if (currentNode.facesEnd && Intersection::intersect(currentNode.bounds, ray)) {
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
                    const Intersection its {triangle, ray};

                    if (its.distance < distance) {
                        *this = its;
                        triangleIndex = i;
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
}
