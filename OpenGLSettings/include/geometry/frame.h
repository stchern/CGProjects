#ifndef FRAME_H
#define FRAME_H

#include "point3d.h"

/**
 * @brief The CameraFrame struct takes a view direction and an up-vector
 * to create a left-handed orthonormal coordinate system.
 */
struct CameraFrame {
    Vector3D dir, right, up; // dir = -z, right = x, up = y
    CameraFrame(const Vector3D& dir, const Vector3D& up)
        : dir{normalize(dir)}, right(normalize(cross(dir, up))), up{cross(this->right, this->dir)}
    {
    }
};

/**
 * @brief The OrthonormalSystem struct creates a right-handed orthonormal system from a normal
 * vector
 */
struct OrthonormalSystem {
    Vector3D s{1.0f, 0.0f, 0.0f}, t{0.0f, 1.0f, 0.0f};
    Normal3D n{0.0f, 0.0f, 1.0f};

    OrthonormalSystem() = default;

    /**
     * @brief OrthonormalSystem
     * @param n normal vector (must be normalized!)
     */
    OrthonormalSystem(Normal3D n)
        : OrthonormalSystem{(std::abs(n.x) > std::abs(n.y)) ? normalize({n.z, 0.0f, -n.x})
                                                            : normalize({0.0f, n.z, -n.y}),
                            n}
    {
    }

    Vector3D toLocal(Vector3D v) const { return {dot(v, s), dot(v, t), dot(v, n)}; }
    Vector3D toWorld(Vector3D v) const { return s * v.x + t * v.y + n * v.z; }

private:
    OrthonormalSystem(Vector3D t, Normal3D n) : s{cross(t, n)}, t{t}, n{n} {}
};

#endif
