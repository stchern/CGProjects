#ifndef BVH_H
#define BVH_H

#include <span>
#include <vector>

#include "aabb.h"
#include "common.h"

class BVH {
public:
    /// desired number of faces per leaf node (the actual number will vary)
    static constexpr uint32_t numFacesPerLeaf{4};
    static_assert(numFacesPerLeaf >= 2, "maxFacesPerLeaf needs to be at least 2");
    /// maximum depth (overrules maximum number of faces)
    static constexpr uint32_t maxDepth{20};
    static_assert(maxDepth >= 1 && maxDepth <= 32, "maxDepth needs to be between 1 and 32");

    struct Node {
        /// bounding box of all contained triangles
        AABB bounds;
        /// first index into faceIndices
        uint32_t facesBegin{0};
        /// one past the last index into faceIndices
        uint32_t facesEnd{0};
    };

    BVH() = default;

    /// construct a BVH for the given mesh (implemented in exercise03.cpp)
    void construct(const Mesh& mesh);

    bool isConstructed() const { return !nodes.empty(); }

    /// return all the nodes in the BVH
    const std::vector<Node>& getNodes() const { return nodes; }
    /// return all face indices belonging to a specific node
    std::span<const uint32_t> getFaceIndices(const Node& node) const
    {
        return {faceIndices.cbegin() + node.facesBegin, faceIndices.cbegin() + node.facesEnd};
    }

    bool isNodeLeaf(size_t currentIdx) const;
private:
    /// return all face indices belonging to a specific node
    std::span<uint32_t> getFaceIndices(const Node& node)
    {
        return {faceIndices.begin() + node.facesBegin, faceIndices.begin() + node.facesEnd};
    }

    /// full binary tree - some nodes might be empty (facesBegin == facesEnd == 0)
    std::vector<Node> nodes;
    /// re-organized array containing all face indices (referenced by the BVH nodes)
    std::vector<uint32_t> faceIndices;
};

#endif // BVH_H
