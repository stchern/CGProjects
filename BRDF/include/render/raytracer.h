#ifndef RAYTRACER_H
#define RAYTRACER_H
#include <atomic>
#include <thread>

#include "camera.h"
#include "film.h"
#include "intersection.h"
#include "ray.h"
#include "scene.h"

enum class RenderMode { ScreenCoord, RayOrigin, RayDirection, Depth, Position, Normal, Whitted };

struct RayTracerParameters {
    RenderMode mode{RenderMode::Whitted};
    /// maximum number of samples per pixel to render (don't burn the CPU too much)
    uint16_t maxSPP{32};
    /// maximum number of ray bounces
    uint16_t maxDepth{6};
};

class RayTracer final {
public:
    RayTracer() = default;
    ~RayTracer() { stop(); }

    /// assign the scene to be rendered
    void setScene(Scene&& scene);
    /// assign the scene to be rendered (copy)
    void setScene(const Scene& scene) { setScene(Scene{scene}); }
    void setParams(RayTracerParameters params)
    {
        stop();
        this->params = params;
    }

    void start(const CameraParameters& cameraParams);
    void stop();

    bool imageHasChanged() const { return new_sample_available.exchange(false); }
    const Scene& getScene() const { return scene; }
    const Camera& getCamera() const { return camera; }
    const Film& getFilm() const { return film; }

    Color depthIntegrator(const Scene& scene, const Ray& ray) const;
    Color positionIntegrator(const Scene& scene, const Ray& ray) const;
    Color normalIntegrator(const Scene& scene, const Ray& ray) const;
    Color whittedIntegrator(const Scene& scene, const Ray& ray, uint32_t depth = 0) const;

    static Color computeDirectLight(const Scene& scene, const ShadingIntersection& its);

private:
    std::thread render_thread{};
    Scene scene{};
    Camera camera{};
    Film film{};
    RayTracerParameters params;
    bool keep_rendering{false};
    mutable std::atomic_bool new_sample_available{false};

private:
    void render();
};

#endif // !RAYTRACER_H
