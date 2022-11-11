#ifndef NORMAL_INTEGRATOR_H
#define NORMAL_INTEGRATOR_H

#include "integrator.h"
#include "ray.h"
#include "scene.h"

class NormalIntegrator : public Integrator {
    using Color = nanogui::Color;

public:
    virtual ~NormalIntegrator() override = default;

    Color L(const Scene& scene, const Ray& ray) const override;
};

#endif // !NORMAL_INTEGRATOR_H
