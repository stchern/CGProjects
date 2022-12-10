#ifndef MESH_H
#define MESH_H

#include <string>
#include <utility>
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
 * @brief The TriangleNormals struct references normals of a mesh to simplify working with a concrete triangle.
 */
struct TriangleNormals {
    // vertices
    const Normal3D &n1, &n2, &n3;
};

/**
 * @brief The BarycentricCoordinates struct contains two barycentric coordinates and computes the third.
 * It can be used to interpolate values within a triangle.
 */
struct BarycentricCoordinates {
    float lambda1() const { return 1.0f - lambda2 - lambda3; }
    float lambda2{0.0f}, lambda3{0.0f};

    Point3D interpolate(const Point3D& v1, const Point3D& v2, const Point3D& v3) const {
        return v1*lambda1()+v2*lambda2+v3*lambda3;
    }
    float interpolate(const float x1, const float x2, const float x3) const {
        return x1*lambda1()+x2*lambda2+x3*lambda3;
    }
    Point3D interpolate(const Triangle& triangle) const {
        return interpolate(triangle.v1, triangle.v2, triangle.v3);
    }
    Normal3D interpolate(const TriangleNormals& triangleNormals) const {
        return interpolate(triangleNormals.n1, triangleNormals.n2, triangleNormals.n3);
    }
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
    /// get total face area
    float getTotalFaceArea() const { return faceAreaPrefixSum.back(); }
    /// get 1 / total face area
    float getInvTotalFaceArea() const { return invTotalArea; }
    /// get the face area prfix sum
    const std::vector<float>& getFaceAreaPrefixSum() const { return faceAreaPrefixSum; }
    /// compute the point and normal in a triangle face for the given barycentric coordinates
    std::pair<Point3D, Normal3D> computePointAndNormal(uint32_t faceIndex,
                                                       const BarycentricCoordinates& bary) const;
    /// uniformly sample a point (and compute its normal) on the mesh.
    std::pair<Point3D, Normal3D> samplePointAndNormal(Point2D sample) const;

    /// get the smooth groups (ranges of faces to be drawn with smooth shading)
    const std::vector<std::pair<size_t, size_t>>& getSmoothGroups() const { return smoothGroups; }

    bool isSmoothFace(uint32_t i) const {
        for (auto [begin, end] : smoothGroups)
            if (i < end)
                return (i >= begin);
        return false;
    }

    /// create a triangle from its vertex indices
    Triangle getTriangleFromFace(const TriangleIndices& indices) const
    {
        return {vertices.at(indices.v1), vertices.at(indices.v2), vertices.at(indices.v3)};
    }
    /// gather the triangle normals from their vertex indices
    TriangleNormals getTriangleNormalsFromFace(const TriangleIndices& indices) const
    {
        return {normals.at(indices.v1), normals.at(indices.v2), normals.at(indices.v3)};
    }

    /// get the triangle that corresponds to the given face index
    Triangle getTriangleFromFaceIndex(size_t i) const { return getTriangleFromFace(faces.at(i)); }
    /// get the triangle normals that correspond to the given face index
    TriangleNormals getTriangleNormalsFromFaceIndex(size_t i) const {
        return getTriangleNormalsFromFace(faces.at(i));
    }

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
    /// the area of this triangle and all preceeding ones
    std::vector<float> faceAreaPrefixSum;
    /// 1 / total face area
    float invTotalArea{};
    /// smooth groups
    std::vector<std::pair<size_t, size_t>> smoothGroups;

    /// Bounding-Volume-Hierarchy
    BVH bvh;
};

#endif // MESH_H
