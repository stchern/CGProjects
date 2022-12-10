#ifndef SAMPLER_H
#define SAMPLER_H

#include <cmath>
#include <random>
#include <common/constants.h>
#include <geometry/point2d.h>
#include <geometry/point3d.h>

class Sampler
{
public:
    /// random
    static float randomFloat() { return randFloat(prng); }
    static Point2D randomSquare() { return { randomFloat(), randomFloat() }; }

    static Vector3D uniformHemisphere() {
        const float cosTheta = randomFloat();
        const float sinTheta = std::sqrt(1.0f-cosTheta*cosTheta);
        const float phi = 2.0f*pi*randomFloat();
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        return { sinTheta*cosPhi, sinTheta*sinPhi, cosTheta };
    }

    static Vector3D cosineHemisphere() {
        const float sinThetaSqr = randomFloat();
        const float cosTheta = std::sqrt(1.0f-sinThetaSqr);
        const float sinTheta = std::sqrt(sinThetaSqr);
        const float phi = 2.0f*pi*randomFloat();
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        return { sinTheta*cosPhi, sinTheta*sinPhi, cosTheta };
    }

    static float uniformHemispherePdf() {
        return 1.0f/(2.0f*pi);
    }

    static float cosineHemispherePdf(float cosTheta) {
        return cosTheta*invPi;
    }

private:
    static thread_local std::mt19937_64 prng;
    static std::uniform_real_distribution<float> randFloat;
};

#endif // SAMPLER_H
