#ifndef RAYTRACER_H
#define RAYTRACER_H
#include <atomic>
#include <thread>

#include "camera.h"
#include "film.h"
#include "intersection.h"
#include "ray.h"
#include "scene.h"

struct RayTracerParameters {
    enum class RenderMode { Depth, Position, Normal, Whitted, Path };

    RenderMode mode{RenderMode::Whitted};
    /// maximum number of samples per pixel to render (don't burn the CPU too much)
    uint16_t maxSPP{32};
    /// maximum number of ray bounces
    uint16_t maxDepth{6};

    bool operator==(const RayTracerParameters& other) const = default;
};

class RayTracer final {
public:
    RayTracer() = default;
    ~RayTracer() { stop(); }

    /// assign the scene to be rendered
    void setScene(Scene&& scene);
    /// assign the scene to be rendered (copy)
    void setScene(const Scene& scene) { setScene(Scene{scene}); }
    bool setParams(const RayTracerParameters params, const CameraParameters& cameraParams);

    void start();
    void stop();

    bool imageHasChanged() const { return new_sample_available.exchange(false); }
    float getSPPRendererd() const { return sppRendered + partialSPPRendered; }

    const Scene& getScene() const { return scene; }
    const Camera& getCamera() const { return camera; }
    const Film& getFilm() const { return film; }

    Color depthIntegrator(const Ray& cameraRay) const;
    Color positionIntegrator(const Ray& cameraRay) const;
    Color normalIntegrator(const Ray& cameraRay) const;
    Color whittedIntegrator(const Ray& cameraRay, uint32_t depth = 0) const;
    Color pathIntegrator(const Ray& cameraRay) const;

    Color computeDirectLight(const ShadingIntersection& its, const Vector3D omegaO,
                             bool pointLightsOnly = false) const;
    std::pair<Vector3D, Color> specularReflection(const Material& material, Vector3D omegaO) const;

private:
    std::thread renderThread{};
    Scene scene{};
    Camera camera{};
    Film film{};
    CameraParameters cameraParams;
    RayTracerParameters params;
    bool keep_rendering{false};
    mutable std::atomic_bool new_sample_available{false};
    uint16_t sppRendered{};
    float partialSPPRendered{};

private:
    void render();
};

#endif // !RAYTRACER_H
