#include "raytracer.h"

#include <iostream>
#include <thread>

#include <nanogui/texture.h>
#include <nanogui/vector.h>

#include "camera.h"
#include "film.h"
#include "scene.h"

using namespace nanogui;

RayTracer::RayTracer(std::unique_ptr<Scene>&& scene) : scene{std::move(scene)} {}

RayTracer::~RayTracer() { stop(); }

void RayTracer::resize(const Vector2i& size, const CameraParameters& cameraParams)
{
    stop();

    film = std::make_unique<Film>(size);
    camera = std::make_unique<Camera>(cameraParams);

    start();
}

void RayTracer::start()
{
    stop();

    if (!camera || !film || !scene || !integrator)
        return;

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
    const Vector2i resolution = film->getResolution();
    const Point2D invResolution = 1.0f / Vector2f{resolution};

    auto radicalInverse = [](const int base, int i) {
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

    int spp = 0;

    while (keep_rendering && spp < max_spp) {
        const Point2D sub_pixel{radicalInverse(2, spp), radicalInverse(3, spp)};

#pragma omp parallel for schedule(dynamic)
        for (int y = 0; y < resolution.y(); ++y) {
            for (int x = 0; x < resolution.x() && keep_rendering; ++x) {
                const Point2D pixel{static_cast<float>(x), static_cast<float>(y)};
                const Ray ray =
                    camera->generateRay(((pixel + sub_pixel) * invResolution - 0.5f) * 2.0f);
                // render pixel position
                // const Color color{pixel.x*invResolution.x, pixel.y*invResolution.y, 0.0f, 1.0f};
                // render ray direction
                // const Color color{ray.direction.x, ray.direction.y, ray.direction.z, 1.0f};
                // use the integrator to determine the pixel color
                const Color color = integrator->L(*scene, ray);
                film->addPixelColor({x, y}, color);
            }
            new_sample_available = true;
        }
        ++spp;
    }
}
