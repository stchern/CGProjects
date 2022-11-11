#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>

#include "aabb.h"
#include "bvh.h"
#include "common.h"
#include "triangle.h"

/**
 * @brief 3D Triangle Mesh
 */
class Mesh {
public:
    Mesh() = default;

    /**
     * @brief loadOBJ loads an OBJ file containing triangles or quads
     * vertex normals and texture coordinates are ignored
     * all faces are merged into one object
     * @param filename
     */
    void loadOBJ(const std::string& filename);

    /// remove all vertices and faces
    void clear() { *this = {}; }

    /// get vertices for reading
    const std::vector<Vertex>& getVertices() const { return vertices; }
    /// get vertices for writing
    std::vector<Vertex>& getVertices() { return vertices; }
    /// get faces for reading
    const std::vector<TriangleIndices>& getFaces() const { return faces; }
    /// get faces for writing
    std::vector<TriangleIndices>& getFaces() { return faces; }
    /// get pre-computed bounding box
    const AABB& getBounds() const { return aabb; }
    /// re-compute bounding box
    void updateBounds();

    /// get normals for reading
    const std::vector<Vertex>& getNormals() const { return normals; }
    /// get normals for writing
    std::vector<Vertex>& getNormals() { return normals; }
    /// get texture coordinates for reading
    const std::vector<TextureCoordinate>& getTextureCoordinates() const { return texCoords; }
    /// get texture coordinates for writing
    std::vector<TextureCoordinate>& getTextureCoordinates() { return texCoords; }

    /// get the face areas
    const std::vector<float>& getFaceAreas() const { return faceAreas; }

    /// get the smooth groups (ranges of faces to be drawn with smooth shading)
    const std::vector<std::pair<size_t, size_t>>& getSmoothGroups() const { return smoothGroups; }

    /// create a triangle from its vertex indices
    Triangle getTriangleFromFace(const TriangleIndices& indices) const
    {
        return {vertices.at(indices.v1), vertices.at(indices.v2), vertices.at(indices.v3)};
    }

    /// get the triangle that corresponds to the given face index
    Triangle getTriangleFromFaceIndex(size_t i) const { return getTriangleFromFace(faces.at(i)); }

    const BVH& getBVH() const { return bvh; }

private:
    /// the vertices of the mesh
    std::vector<Vertex> vertices;
    /// the triangle faces of the mesh
    std::vector<TriangleIndices> faces;
    /// axis aligned bounding box containing all vertices
    AABB aabb;

    /// the normal vectors per vertex
    std::vector<Vertex> normals;
    /// the texture coordinates per vertex
    std::vector<TextureCoordinate> texCoords;
    /// the area of each triangle
    std::vector<float> faceAreas;
    /// smooth groups
    std::vector<std::pair<size_t, size_t>> smoothGroups;

    /// Bounding-Volume-Hierarchy
    BVH bvh;
};

#endif // MESH_H
