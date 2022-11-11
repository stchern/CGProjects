#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "common.h"

/**
 * @brief Usually, a triangle is only represented by the indices of its vertex.
 */
struct TriangleIndices {
    uint32_t v1, v2, v3;
};

/**
 * @brief The Triangle struct references vertices of a mesh to simplify working a concrete triangle.
 */
struct Triangle {
    // vertices
    const Vertex &v1, &v2, &v3;
    // edges
    const Vector3D v1v2{v2 - v1};
    const Vector3D v1v3{v3 - v1};
};

#endif // !TRIANGLE_H
