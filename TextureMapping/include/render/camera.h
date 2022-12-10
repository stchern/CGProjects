#ifndef CAMERA_H
#define CAMERA_H

#include <common/constants.h>
#include <geometry/frame.h>
#include <geometry/matrix3d.h>
#include <geometry/matrix4d.h>
#include <geometry/point2d.h>
#include <geometry/point3d.h>

#include "film.h"
#include "ray.h"

/**
 * @brief The CameraParameters struct contains all parameters necessary to configure a camera.
 */
struct CameraParameters {
    /// 3D position of the camera
    Point3D pos{0.0f, 1.0f, 3.0f};
    /// 3D position of the target to look at
    Point3D target{0.0f, 1.0f, 0.0f};
    /// up vector
    Vector3D up{0.0f, 1.0f, 0.0f};

    struct Perspective {
        /// field of view in degrees
        float fov{45.0f};

        bool operator==(const Perspective& other) const = default;
    } perspective{};

    struct Orthographic {
        float left{-1.0f};
        float right{1.0f};
        float top{1.0f};
        float bottom{-1.0f};

        bool operator==(const Orthographic& other) const = default;
    } orthographic{};

    Resolution resolution{128, 128};

    /// near clipping plane
    float tNear{0.1f};
    /// far clipping plane
    float tFar{100.0f};

    /// apsect ratio
    float aspect() const
    {
        return static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
    }

    enum class CameraType { Perspective, Orthographic } type{CameraType::Perspective};

    /// orthonormal camera frame
    CameraFrame computeFrame() const { return {target - pos, up}; }
    /// half of the width and height of the view frustum at depth 1
    Point2D halfViewSpan() const
    {
        float w = std::tan(perspective.fov * degToRad * 0.5f);
        return {w * aspect(), w};
    }

    /// view transformation matrix for OpenGL
    HomogeneousTransformation3D viewTransformation() const
    {
        const CameraFrame frame = computeFrame();
        Matrix3D rotation = Matrix3D{frame.right, frame.up, -frame.dir};
        rotation = rotation.transposed();
        const Vector3D translation = rotation * -pos;
        return HomogeneousTransformation3D{rotation, translation};
    }

    /// orthographic projection matrix for OpenGL
    HomogeneousTransformation3D orthographicProjection() const
    {
        const Vector3D scale = Vector3D{orthographic.right - orthographic.left,
                                        orthographic.top - orthographic.bottom, tNear - tFar}
                                   .inverse()
                             * 2.0f;
        const Vector3D translation{-(orthographic.left + orthographic.right) * 0.5f,
                                   -(orthographic.bottom + orthographic.top) * 0.5f,
                                   (tFar + tNear) * 0.5f};
        return {Matrix3D::scale(scale), scale * translation};
    }

    /// perspective projection matrix for OpenGL
    Matrix4D perspecitveProjection() const
    {
        const Point2D invHalfFocusPlaneSize = halfViewSpan().inverse();
        // perspective transformation
        return {{invHalfFocusPlaneSize.x, 0.0f, 0.0f, 0.0f},
                {0.0f, invHalfFocusPlaneSize.y, 0.0f, 0.0f},
                {0.0f, 0.0f, (tNear + tFar) / (tNear - tFar), -1.0f},
                {0.0f, 0.0f, 2.0f * tNear * tFar / (tNear - tFar), 0.0f}};
    }

    Matrix4D projection() const
    {
        if (type == CameraType::Perspective)
            return perspecitveProjection();
        else
            return orthographicProjection();
    }

    bool operator==(const CameraParameters& other) const = default;
};

/**
 * A simple pinhole camera model that is used to generate rays.
 */
class Camera {
    using CameraType = CameraParameters::CameraType;
    using Orthographic = CameraParameters::Orthographic;

public:
    Camera(const CameraParameters& params = {})
        : origin{params.pos}, cameraFrame{params.target - params.pos, params.up},
          halfViewSpan{params.halfViewSpan()}, orthographic{params.orthographic}, type{params.type},
          tNear{params.tNear}, tFar{params.tFar}
    {
    }

    /**
     * @brief generateRay
     * @param pos in normalized sceen coordinates [-1,1]^2, x is towards the right, y is up
     * @return camera ray
     */
    Ray generateRay(Point2D pos) const
    {
        switch (type) {
        case CameraType::Perspective:
            return perspectiveRay(pos);
        case CameraType::Orthographic:
            return orthographicRay(pos);
        }
        return {};
    }

    Ray perspectiveRay(Point2D pos) const
    {
        const Point2D nearPlanePixel = pos * halfViewSpan;
        const Vector3D direction = normalize(cameraFrame.dir + cameraFrame.right * nearPlanePixel.x
                                             - cameraFrame.up * nearPlanePixel.y);
        const float invZ = 1.0f / dot(direction, cameraFrame.dir);
        return Ray{origin, direction, tNear * invZ, tFar * invZ};
    }

    Ray orthographicRay(Point2D pos) const
    {
        Point2D orthoSize{orthographic.right - orthographic.left,
                          orthographic.top - orthographic.bottom};
        Point2D orthoOffset{orthographic.right / orthoSize.x - 0.5f,
                            -orthographic.bottom / orthoSize.y - 0.5f};

        Point2D cameraPlanePixel = (pos * 0.5f + orthoOffset) * orthoSize;
        Vector3D direction =
            cameraFrame.right * cameraPlanePixel.x - cameraFrame.up * cameraPlanePixel.y;
        return Ray{origin + direction, cameraFrame.dir, tNear, tFar};
    }

private:
    Point3D origin;
    CameraFrame cameraFrame;
    /// half of the width and height of the view frustum at depth 1
    Vector2D halfViewSpan;
    Orthographic orthographic;
    CameraType type{CameraType::Perspective};
    float tNear, tFar;
};

#endif // CAMERA_H
