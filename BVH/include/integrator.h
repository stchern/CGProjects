#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "ray.h"
#include "scene.h"

class Integrator {
public:
    virtual nanogui::Color L(const Scene& scene, const Ray& ray) const = 0;
    virtual ~Integrator() = default;
};

#endif // !INTEGRATOR_H
