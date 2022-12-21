#ifndef SAMPLER_H
#define SAMPLER_H

#include <cmath>
#include <common/constants.h>
#include <geometry/point2d.h>
#include <random>

class Sampler {
public:
    /// random
    static float randomFloat() { return randFloat(prng); }
    static Point2D randomSquare() { return {randomFloat(), randomFloat()}; }

private:
    static thread_local std::mt19937_64 prng;
    static std::uniform_real_distribution<float> randFloat;
};

#endif // SAMPLER_H
