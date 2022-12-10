#ifndef LIGHT_H
#define LIGHT_H

#include "color.h"
#include "ray.h"
#include <common/constants.h>
#include <geometry/point3d.h>

class PointLight {
public:
    Color power;
    Point3D pos;

    Color Li(Point3D x) const
    {
        float dist = distance(x, pos);
        return power / (4 * pi * dist * dist);
    }

    Ray shadowRay(Point3D x) const
    {
        const Vector3D dist = pos - x;
        float norm = dist.norm();
        return {x, dist / norm, epsilon, norm * (1.0f - epsilon)};
    }
};

#endif // LIGHT_H
