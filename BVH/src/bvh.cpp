#include "bvh.h"

#include "intersection.h"
#include "mesh.h"
#include "ray.h"
#include "triangle.h"

#include <algorithm>
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
        uint32_t numNodes = roundUpPowerOfTwo(numFaces / numFacesPerLeaf);
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

    // TODO: build the BVH
    for (size_t nodeIdx = 0; nodeIdx < numFaces / 2 - 1; ++nodeIdx) {
        if ((nodes[nodeIdx].facesEnd - nodes[nodeIdx].facesBegin > 4) && (nodeIdx < std::pow(2, maxDepth) - 1)) {
            const uint32_t start = nodes[nodeIdx].facesBegin;
            const uint32_t end = nodes[nodeIdx].facesEnd;
            const uint32_t mid = (end + start) / 2;
            const uint32_t indexOfMaxDimension = nodes[nodeIdx].bounds.extents().maxDimension();

            std::nth_element(std::begin(faceIndices) + start, std::begin(faceIndices) + mid, std::begin(faceIndices) + end,
                        [indexOfMaxDimension, triangleCenter](uint32_t triangleIndexLhs, uint32_t triangleIndexRhs) {
                return triangleCenter(triangleIndexLhs, indexOfMaxDimension) < triangleCenter(triangleIndexRhs, indexOfMaxDimension);
                }
            );

            AABB leftChildAABB;
            for (uint32_t idx = start; idx < mid; ++idx) {
                const Triangle triangle = mesh.getTriangleFromFaceIndex(faceIndices[idx]);
                leftChildAABB.extend(triangle.v1);
                leftChildAABB.extend(triangle.v2);
                leftChildAABB.extend(triangle.v3);
            }

            AABB rightChildAABB;
            for (uint32_t idx = mid; idx < end; ++idx) {
                const Triangle triangle = mesh.getTriangleFromFaceIndex(faceIndices[idx]);
                rightChildAABB.extend(triangle.v1);
                rightChildAABB.extend(triangle.v2);
                rightChildAABB.extend(triangle.v3);
            }

            nodes.push_back(Node{leftChildAABB, start, mid});
            nodes.push_back(Node{rightChildAABB, mid, end});
        }
        else {
            AABB emptyChildAABB;
            nodes.push_back(Node{emptyChildAABB, 0, 0});
            nodes.push_back(Node{emptyChildAABB, 0, 0});
        }
    }

    std::cout << "done. (used " << nodes.size() << " BVH nodes)" << std::endl;
}



bool BVH::isNodeLeaf(size_t currentIdx) const
{
    if (currentIdx * 2 + 1 >= nodes.size())
        return true;

    if (nodes[currentIdx * 2 + 1].facesEnd == 0)
        return true;
    return false;
}
