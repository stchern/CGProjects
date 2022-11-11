#ifndef CAMERA_H
#define CAMERA_H

#include "camera_parameters.h"
#include "common.h"
#include "ray.h"

/**
 * A simple pinhole camera model, that is used to generate rays.
 */
class Camera {
    using Orthographic = CameraParameters::Orthographic;

public:
    Camera(const CameraParameters& params)
        : origin{params.pos}, cameraFrame{params.target - params.pos, params.up},
          halfViewSpan{params.halfViewSpan()}, orthographic{params.orthographic},
          isPerspective{params.type == CameraParameters::CameraType::Perspective}
    {
    }

    /**
     * @brief generateRay
     * @param pos in normalized sceen coordinates [-1,1]^2, x is towards the right, y is up
     * @return camera ray
     */
    Ray generateRay(Point2D pos) const
    {
        if (isPerspective)
            return perspectiveRay(pos);
        else
            return orthographicRay(pos);
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
                            orthographic.top / orthoSize.y - 0.5f};

        Point2D cameraPlanePixel = (pos * 0.5f + orthoOffset) * orthoSize;
        Vector3D direction =
            cameraFrame.right * cameraPlanePixel.x - cameraFrame.up * cameraPlanePixel.y;
        return Ray{origin + direction, cameraFrame.dir};
    }

private:
    Point3D origin;
    CameraFrame cameraFrame;
    /// half of the width and height of the view frustum at depth 1
    Point2D halfViewSpan;
    Orthographic orthographic;
    bool isPerspective{true};
};
#endif // CAMERA_H
