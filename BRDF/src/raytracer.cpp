#include <render/raytracer.h>

#include <render/camera.h>
#include <render/color.h>
#include <render/film.h>
#include <render/intersection.h>
#include <render/scene.h>

Color RayTracer::depthIntegrator(const Scene& scene, const Ray& ray) const
{
    ShadingIntersection its = intersect(scene, ray);
    if (!its)
        return {};

    AABB bounds{scene.getBounds()};
    bounds.extend(ray.origin);
    return {its.distance / bounds.extents().maxComponent()};
}

Color RayTracer::positionIntegrator(const Scene& scene, const Ray& ray) const
{
    ShadingIntersection its = intersect(scene, ray);
    if (!its)
        return {};

    return {(its.point - scene.getBounds().min) / scene.getBounds().extents()};
}

Color RayTracer::normalIntegrator(const Scene& scene, const Ray& ray) const
{
    ShadingIntersection its = intersect(scene, ray);
    if (!its)
        return {};

    return {its.shadingFrame.n * 0.5f + 0.5f};
}

Color RayTracer::whittedIntegrator(const Scene& scene, const Ray& ray, uint32_t depth) const
{
    if (depth == params.maxDepth)
        return {};

    ShadingIntersection its = intersect(scene, ray);
    if (!its)
        return {};

    Color result{0.0f};

    auto toWorld = [&its](const auto& v) { return its.shadingFrame.toWorld(v); };
    auto toLocal = [&its](const auto& v) { return its.shadingFrame.toLocal(v); };

    const Material& material = its.instance->material;

    if (material.isMirror()) {
        const Vector3D omegaO = toLocal(-ray.direction);
        const Vector3D omegaI = Material::reflect(omegaO);
        const Ray reflectionRay{its.point, toWorld(omegaI)};
        result = whittedIntegrator(scene, reflectionRay, depth + 1);
    }
    else if (material.isGlass()) {
        const Vector3D omegaO = toLocal(-ray.direction);
        const Vector3D omegaI = Material::reflect(omegaO);
        const Vector3D omegaT = material.dielectric().refract(omegaO);
        const float fresnel = material.dielectric().fresnel(omegaO.z, omegaT.z);
        const Ray reflectionRay{its.point, toWorld(omegaI)};
        result = whittedIntegrator(scene, reflectionRay, depth + 1) * fresnel;
        if (fresnel < 1.0f) {
            const Ray refractionRay{its.point, toWorld(omegaT)};
            result += whittedIntegrator(scene, refractionRay, depth + 1) * (1.0f - fresnel);
        }
    }
    else if (material.isConductor()) {
        const Vector3D omegaO = toLocal(-ray.direction);
        const Color fresnel = material.conductor().fresnel(omegaO.z);
        const Vector3D omegaI = Material::reflect(omegaO);
        const Ray reflectionRay{its.point, toWorld(omegaI)};
        result = whittedIntegrator(scene, reflectionRay, depth + 1) * fresnel;
    }
    else if (material.isDiffuse())
        result += computeDirectLight(scene, its);

    result.a = 1.0f;

    return result;
}

void RayTracer::setScene(Scene&& scene)
{
    stop();

    this->scene = std::move(scene);
}

void RayTracer::start(const CameraParameters& cameraParams)
{
    stop();

    camera = {cameraParams};
    film = {cameraParams.resolution};

    keep_rendering = true;
    render_thread = std::thread(&RayTracer::render, this);
}

void RayTracer::stop()
{
    keep_rendering = false;
    if (render_thread.joinable())
        render_thread.join();
}

void RayTracer::render()
{
    const Resolution resolution = film.getResolution();
    const Point2D invResolution = {1.0f / resolution.x, 1.0f / resolution.y};

    auto radicalInverse = [](const uint32_t base, uint32_t i) {
        float digit, radical, inverse;
        digit = radical = 1.0f / static_cast<float>(base);
        inverse = 0.0f;
        while (i) {
            inverse += digit * static_cast<float>(i % base);
            digit *= radical;
            i /= base;
        }

        return inverse;
    };

    uint32_t spp = 0;

    while (keep_rendering && spp < params.maxSPP) {
        const Point2D sub_pixel{radicalInverse(2, spp), radicalInverse(3, spp)};

#pragma omp parallel for schedule(dynamic)
        for (uint32_t y = 0; y < resolution.y; ++y) {
            for (uint32_t x = 0; x < resolution.x && keep_rendering; ++x) {
                const Pixel pixel{x, y};
                const Point2D normalizedScreenCoords =
                    ((Point2D{pixel} + sub_pixel) * invResolution - 0.5f) * 2.0f;
                const Ray ray = camera.generateRay(normalizedScreenCoords);

                Color color;
                switch (params.mode) {
                case RenderMode::ScreenCoord:
                    color = {normalizedScreenCoords.x, normalizedScreenCoords.y, 0.0f};
                    break;
                case RenderMode::RayOrigin:
                    color = ray.origin;
                    break;
                case RenderMode::RayDirection:
                    color = ray.direction;
                    break;
                case RenderMode::Depth:
                    color = depthIntegrator(scene, ray);
                    break;
                case RenderMode::Position:
                    color = positionIntegrator(scene, ray);
                    break;
                case RenderMode::Normal:
                    color = normalIntegrator(scene, ray);
                    break;
                case RenderMode::Whitted:
                    color = whittedIntegrator(scene, ray);
                    break;
                }
                film.addPixelColor(pixel, color);
            }
            new_sample_available = true;
        }
        ++spp;
    }
}
