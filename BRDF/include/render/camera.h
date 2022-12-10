#ifndef CAMERA_H
#define CAMERA_H

#include <common/constants.h>
#include <geometry/point2d.h>
#include <geometry/point3d.h>

#include "film.h"
#include "ray.h"

#include <stdexcept>

/**
 * @brief The CameraFrame class takes a view direction and an up-vector
 * to create an orthonormal coordinate system.
 */
struct CameraFrame {
    Vector3D dir, right, up; // dir = -z, right = x, up = y
    CameraFrame(const Vector3D& dir, const Vector3D& up)
        : dir{normalize(dir)}, right(normalize(cross(dir, up))), up{cross(this->right, this->dir)}
    {
    }
};

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
        float fov{30.0f};
    } perspective;

    struct Orthographic {
        float left{-1.0f};
        float right{1.0f};
        float top{1.0f};
        float bottom{-1.0f};
    } orthographic;

    Resolution resolution{128, 128};

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
          halfViewSpan{params.halfViewSpan()}, orthographic{params.orthographic}, type{params.type}
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
        default:
            throw std::logic_error("unhandled camera type");
        }
    }

    Ray perspectiveRay(Point2D pos) const
    {
        Point2D nearPlanePixel = pos * halfViewSpan;
        Vector3D direction = normalize(cameraFrame.dir + cameraFrame.right * nearPlanePixel.x
                                       - cameraFrame.up * nearPlanePixel.y);
        return Ray{origin, direction};
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
        return Ray{origin + direction, cameraFrame.dir};
    }

private:
    Point3D origin;
    CameraFrame cameraFrame;
    /// half of the width and height of the view frustum at depth 1
    Vector2D halfViewSpan;
    Orthographic orthographic;
    CameraType type{CameraType::Perspective};
};

#endif // CAMERA_H
