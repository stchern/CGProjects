#ifndef LIGHT_H
#define LIGHT_H

#include "color.h"
#include "instance.h"
#include "sampler.h"
#include <common/constants.h>
#include <geometry/point3d.h>
#include <utility>
#include <variant>

struct ShadingIntersection;


struct Light {
    struct Point {
        Color power;
        Point3D pos;

        Color Li(Point3D receiver) const { return power / (4.0f * pi * distanceSqr(receiver, pos)); }
    };

    struct Area {
        const Instance instance;

        /// sample a direction towards the emitter and compute the incident radiance
        /// divided by the sampling probability of the produced sample
        std::pair<Color, Point3D> sampleLi(Point3D receiver, Point2D sample) const;
    };

    std::variant<Point, Area> params;

    // check what type of light this is

    bool isPoint() const { return std::holds_alternative<Point>(params); }
    bool isArea() const { return std::holds_alternative<Area>(params); }

    // get a specific light (make sure it is of that type first!)

    Point point() const { return std::get<Point>(params); }
    Area area() const { return std::get<Area>(params); }

    /// sample a direction towards the emitter and compute the incident radiance
    /// divided by the sampling probability of the produced sample
    std::pair<Color, Point3D> sampleLi(Point3D receiver) const {
        if (isPoint())
            return {point().Li(receiver), point().pos};
        else if (isArea())
            return area().sampleLi(receiver, Sampler::randomSquare());
        return {};
    }
};

#endif // LIGHT_H
