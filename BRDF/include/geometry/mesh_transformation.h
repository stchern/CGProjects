#ifndef MESH_TRANSFORMATION_H
#define MESH_TRANSFORMATION_H

#include <geometry/mesh.h>

/**
 * @brief The MeshTransformation class provides some basic transformation operations
 * and can apply these transfromations to an entire mesh.
 */
class MeshTransformation {
public:
    MeshTransformation(const Mesh& mesh) : mesh{mesh}, transformedMesh{mesh} {}

    // transform a single point

    [[nodiscard]] static Point3D scale(Point3D v, float sx, float sy, float sz);

    [[nodiscard]] static Point3D translate(Point3D v, float tx, float ty, float tz);

    [[nodiscard]] static Point3D rotateX(Point3D v, float angle);
    [[nodiscard]] static Point3D rotateY(Point3D v, float angle);
    [[nodiscard]] static Point3D rotateZ(Point3D v, float angle);

    // transform the mesh

    void scaleMesh(float sx, float sy, float sz)
    {
        for (auto& vertex : transformedMesh.getVertices())
            vertex = scale(vertex, sx, sy, sz);
        for (auto& normal : transformedMesh.getNormals())
            normal = scale(normal, 1.0f / sx, 1.0f / sy, 1.0f / sz);
        transformedMesh.updateBounds();
    }
    void translateMesh(float tx, float ty, float tz)
    {
        for (auto& vertex : transformedMesh.getVertices())
            vertex = translate(vertex, tx, ty, tz);
        transformedMesh.updateBounds();
    }
    void rotateMeshX(float angle)
    {
        for (auto& vertex : transformedMesh.getVertices())
            vertex = rotateX(vertex, angle * degToRad);
        for (auto& normal : transformedMesh.getNormals())
            normal = rotateX(normal, angle * degToRad);
        transformedMesh.updateBounds();
    }
    void rotateMeshY(float angle)
    {
        for (auto& vertex : transformedMesh.getVertices())
            vertex = rotateY(vertex, angle * degToRad);
        for (auto& normal : transformedMesh.getNormals())
            normal = rotateY(normal, angle * degToRad);
        transformedMesh.updateBounds();
    }
    void rotateMeshZ(float angle)
    {
        for (auto& vertex : transformedMesh.getVertices())
            vertex = rotateZ(vertex, angle * degToRad);
        for (auto& normal : transformedMesh.getNormals())
            normal = rotateZ(normal, angle * degToRad);
        transformedMesh.updateBounds();
    }
    void resetMesh() { transformedMesh = mesh; }

    const Mesh& getTransformedMesh() const { return transformedMesh; }

private:
    const Mesh& mesh;
    Mesh transformedMesh;
};

#endif // MESH_TRANSFORMATION_H
