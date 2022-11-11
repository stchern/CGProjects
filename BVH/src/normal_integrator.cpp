#include "normal_integrator.h"
#include "intersection.h"

using namespace nanogui;

Color NormalIntegrator::L(const Scene& scene, const Ray& ray) const
{
    // return {ray.direction.x, ray.direction.y, ray.direction.z, 1.0f};
    // return {ray.origin.x, ray.origin.y, ray.origin.z, 1.0f};

    Intersection intersection = intersect(scene, ray);
    if (intersection) {
        Color result{intersection.normal * 0.5f + 0.5f, 1.f};
        return result;
    }
    return {0.f, 0.f, 0.f, 0.f};
}
