#ifndef RAYTRACER_H
#define RAYTRACER_H
#include <atomic>
#include <nanogui/texture.h>
#include <thread>

#include "camera_parameters.h"
#include "common.h"
#include "integrator.h"
#include "normal_integrator.h"

class RayTracer final {
public:
    RayTracer(std::unique_ptr<Scene>&& scene);

    ~RayTracer();

    RayTracer& operator=(const RayTracer&) = delete;

    void start();
    void stop();
    void resize(const nanogui::Vector2i& size, const CameraParameters& cameraParams);

    bool imageHasChanged() const { return new_sample_available.exchange(false); }
    const Film& getFilm() const { return *film; }

private:
    std::thread render_thread;
    std::unique_ptr<Integrator> integrator{std::make_unique<NormalIntegrator>()};
    std::unique_ptr<Scene> scene;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Film> film;
    bool keep_rendering{false};
    mutable std::atomic_bool new_sample_available{false};

    /// maximum number of samples per pixel to render (don't burn the CPU too much)
    static const int max_spp{32};

private:
    void render();
};

#endif // !RAYTRACER_H
