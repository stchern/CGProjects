#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>

#include "aabb.h"
#include "bvh.h"
#include "point2d.h"

/**
 * @brief Usually, a triangle is only represented by the indices of its vertex.
 */
struct TriangleIndices {
    uint32_t v1, v2, v3;
};

/**
 * @brief The Triangle struct references vertices of a mesh to simplify working with a concrete
 * triangle.
 */
struct Triangle {
    // vertices
    const Point3D &v1, &v2, &v3;
    // edges
    const Vector3D v1v2{v2 - v1};
    const Vector3D v1v3{v3 - v1};
};

/**
 * @brief 3D Triangle Mesh
 */
class Mesh {
public:
    Mesh() = default;

    Mesh(const std::string& filename) { loadOBJ(filename); }

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
    const std::vector<Point3D>& getVertices() const { return vertices; }
    /// get vertices for writing
    std::vector<Point3D>& getVertices() { return vertices; }
    /// get faces for reading
    const std::vector<TriangleIndices>& getFaces() const { return faces; }
    /// get faces for writing
    std::vector<TriangleIndices>& getFaces() { return faces; }
    /// get pre-computed bounding box
    const AABB& getBounds() const { return aabb; }
    /// re-compute bounding box
    void updateBounds();

    /// get normals for reading
    const std::vector<Point3D>& getNormals() const { return normals; }
    /// get normals for writing
    std::vector<Point3D>& getNormals() { return normals; }
    /// get texture coordinates for reading
    const std::vector<Point2D>& getTextureCoordinates() const { return texCoords; }
    /// get texture coordinates for writing
    std::vector<Point2D>& getTextureCoordinates() { return texCoords; }

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
    std::vector<Point3D> vertices;
    /// the triangle faces of the mesh
    std::vector<TriangleIndices> faces;
    /// axis aligned bounding box containing all vertices
    AABB aabb;

    /// the normal vectors per vertex
    std::vector<Normal3D> normals;
    /// the texture coordinates per vertex
    std::vector<Point2D> texCoords;
    /// the area of each triangle
    std::vector<float> faceAreas;
    /// smooth groups
    std::vector<std::pair<size_t, size_t>> smoothGroups;

    /// Bounding-Volume-Hierarchy
    BVH bvh;
};

#endif // MESH_H
