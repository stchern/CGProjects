#include <render/raytracer.h>

#include <render/camera.h>
#include <render/color.h>
#include <render/film.h>
#include <render/intersection.h>
#include <render/sampler.h>
#include <render/scene.h>

Color RayTracer::depthIntegrator(const Scene& scene, const Ray& cameraRay) const
{
    const Intersection its{scene, cameraRay};
    if (!its)
        return {};

    AABB bounds{scene.getBounds()};
    bounds.extend(cameraRay.origin);
    return {its.distance / bounds.extents().maxComponent()};
}

Color RayTracer::positionIntegrator(const Scene& scene, const Ray& cameraRay) const
{
    const ShadingIntersection its{scene, cameraRay};
    if (!its)
        return {};

    return Color{(its.point - scene.getBounds().min) / scene.getBounds().extents()};
}

Color RayTracer::normalIntegrator(const Scene& scene, const Ray& cameraRay) const
{
    const ShadingIntersection its{scene, cameraRay};
    if (!its)
        return {};

    return Color{its.shadingFrame.n * 0.5f + 0.5f};
}

Color RayTracer::whittedIntegrator(const Scene& scene, const Ray& cameraRay, uint32_t depth) const
{
    if (depth >= params.maxDepth)
        return {};

    const ShadingIntersection its{scene, cameraRay};
    if (!its)
        return {};

    Color result{0.0f};

    auto toWorld = [&its](const auto& v) { return its.shadingFrame.toWorld(v); };
    auto toLocal = [&its](const auto& v) { return its.shadingFrame.toLocal(v); };

    const Material& material = scene.getInstances().at(its.instanceIndex).material;
    const Vector3D omegaO = toLocal(-cameraRay.direction);

    if (omegaO.z < 0.0f && !material.isDielectric())
        return result;

    if (material.isDiffuse()) {
        result = material.emittedRadiance;
        if (depth + 1 < params.maxDepth)
            result += computeDirectLight(its, omegaO, true);
    }
    else {
        // on these surfaces, the outgoing ray direction is fixed - we cannot sample it
        const auto [omegaI, fresnel] = specularReflection(material, omegaO);
        const Ray reflectionRay{its.point, toWorld(omegaI)};
        result = whittedIntegrator(scene, reflectionRay, depth + 1) * fresnel;
    }

    result.a = 1.0f;

    return result;
}

Color RayTracer::pathIntegrator(const Scene& scene, const Ray& cameraRay) const
{
    Ray ray{cameraRay};

    ShadingIntersection its{scene, ray};
    if (!its)
        return {};

    auto toWorld = [&its](const auto& v) { return its.shadingFrame.toWorld(v); };
    auto toLocal = [&its](const auto& v) { return its.shadingFrame.toLocal(v); };

    /// the final radiance arriving at the camera
    Color result{0.0f};
    /// attenuation of emitted radiance due to previous intersections (BRDF*cosine/PDF)
    Color throughput{1.0f};

    /// sample area lights or just compute their contribution via brute-force path tracing
    const bool sampleAreaLights = true;
    bool diffuse = false;

    for (uint32_t depth = 1; its && depth <= params.maxDepth; ++depth) {

        /// the material at the current intersection
        const Material& material = scene.getInstances().at(its.instanceIndex).material;
        /// the direction towards the camera
        const Vector3D omegaO = toLocal(-ray.direction);

        // backfaces are black
        if (omegaO.z < 0.0f && !material.isDielectric())
            break;

        // potentially add emitted radiance from an emitter
        // (but only if we did not sample direct light at the previous intersection)
        if ((!sampleAreaLights || !diffuse) && material.isEmitter())
            result += throughput * material.emittedRadiance;

        diffuse = material.isDiffuse() || material.isRoughConductor();

        if (diffuse) {
            // on diffuse surfaces, we can sample direct light
            if (depth < params.maxDepth)
                result += throughput * computeDirectLight(its, omegaO, !sampleAreaLights);

            const Vector3D omegaI = Sampler::uniformHemisphere();
            ray = {its.point, toWorld(omegaI)};
            throughput *=
                material.eval(omegaO, omegaI) * omegaI.z * (1.0f / Sampler::uniformHemispherePdf());
        }
        else {
            // on these surfaces, the outgoing ray direction is fixed - we cannot sample it
            const auto [omegaI, fresnel] = specularReflection(material, omegaO);
            ray = {its.point, toWorld(omegaI)};
            throughput *= fresnel;
        }

        if (throughput.isBlack())
            break;

        its = {scene, ray};
    }

    result.a = 1.0f;

    return result;
}

Color RayTracer::computeDirectLight(const ShadingIntersection& its, const Vector3D omegaO,
                                    bool pointLightsOnly) const
{
    const Material& material = scene.getInstances().at(its.instanceIndex).material;
    Color result{0.0f};

    for (const auto& light : scene.getLights()) {
        if (pointLightsOnly && !light.isPoint())
            continue;

        const auto [Li, pos] = light.sampleLi(its.point);
        const Ray shadowRay = Ray::shadowRay(its.point, pos);
        const Vector3D omegaI = its.shadingFrame.toLocal(shadowRay.direction);
        if (omegaI.z <= 0.0f || Intersection{scene, shadowRay})
            continue;

        result += Li * material.eval(omegaO, omegaI) * omegaI.z;
    }

    return result;
}

std::pair<Vector3D, Color> RayTracer::specularReflection(const Material& material,
                                                         Vector3D omegaO) const
{
    // there is a mirror-like reflection involved, so compute it for all cases
    const Vector3D omegaI = Material::reflect(omegaO);

    if (material.isConductor() || material.isRoughConductor()) {
        const Material::Conductor& conductor =
            material.isConductor() ? material.conductor() : material.roughConductor().conductor;
        const Color fresnel = conductor.fresnel(omegaO.z);
        return {omegaI, fresnel};
    }
    else if (material.isDielectric()) {
        const Material::Dielectric& dielectric = material.dielectric();
        const Vector3D omegaT = dielectric.refract(omegaO);
        const float fresnel = dielectric.fresnel(omegaO.z, omegaT.z);
        // randomly choose between reflection and refraction
        if (fresnel < 1.0f && Sampler::randomFloat() >= fresnel)
            return {omegaT, 1.0f};
        return {omegaI, 1.0f};
    }
    else { // if (material.isMirror())
        return {omegaI, 1.0f};
    }
}

void RayTracer::setScene(Scene&& scene)
{
    stop();

    this->scene = std::move(scene);
}

bool RayTracer::setParams(const RayTracerParameters params, const CameraParameters& cameraParams)
{
    RayTracerParameters temp{params};
    temp.maxSPP = this->params.maxSPP;
    const bool needRestart = this->params != temp || this->cameraParams != cameraParams
                          || params.maxSPP < sppRendered
                          || (params.maxSPP > sppRendered && sppRendered == this->params.maxSPP);

    if (needRestart) {
        stop();

        this->cameraParams = cameraParams;
        camera = {cameraParams};
        if (film.getResolution() != cameraParams.resolution)
            film = {cameraParams.resolution};
        else
            film.clearWeights();
    }

    this->params = params;

    return needRestart;
}

void RayTracer::start()
{
    stop();

    keep_rendering = true;
    renderThread = std::thread(&RayTracer::render, this);
}

void RayTracer::stop()
{
    keep_rendering = false;
    if (renderThread.joinable())
        renderThread.join();
}

void RayTracer::render()
{
    const Resolution resolution = film.getResolution();
    const Point2D invResolution = Point2D{resolution}.inverse();

    auto radicalInverse = [](const uint32_t base, uint32_t i) {
        const float radical = 1.0f / static_cast<float>(base);
        float digit = radical;
        float inverse = 0.0f;
        while (i) {
            inverse += digit * static_cast<float>(i % base);
            digit *= radical;
            i /= base;
        }

        return inverse;
    };

    auto blockPos = [](uint32_t blockSize, uint32_t blockSampleIndex) -> Pixel {
        // 0, 1, 0, 1
        auto grey0 = [](uint32_t x) -> uint32_t { return x & 1; };
        // 0, 1, 1, 0
        auto grey1 = [](uint32_t x) -> uint32_t { return (x ^ (x >> 1)) & 1; };

        uint32_t lowX = 0;
        uint32_t lowY = 0;
        for (uint32_t i = 0; (blockSize >> (i + 1)); ++i) {
            if (i & 1) {
                lowX += grey0(blockSampleIndex >> (2 * i)) * (blockSize >> (i + 1));
                lowY += grey1(blockSampleIndex >> (2 * i)) * (blockSize >> (i + 1));
            }
            else {
                lowX += grey1(blockSampleIndex >> (2 * i)) * (blockSize >> (i + 1));
                lowY += grey0(blockSampleIndex >> (2 * i)) * (blockSize >> (i + 1));
            }
        }

        return {lowX, lowY};
    };

    uint32_t blockResDivider = 1;
    sppRendered = 0;
    partialSPPRendered = 0.0f;

    while (keep_rendering && sppRendered < params.maxSPP) {
        const Point2D sub_pixel{radicalInverse(2, sppRendered), radicalInverse(3, sppRendered)};

        const uint32_t blockSize = 32;
        const Resolution numBlocks{(resolution.x + blockSize - 1) / blockSize,
                                   (resolution.y + blockSize - 1) / blockSize};
        const float invNumBlock = 1.0f / (numBlocks.x * numBlocks.y);

        uint32_t index = 0;

        for (--blockResDivider; blockSize >> blockResDivider; ++blockResDivider) {
            const uint32_t pixelSize = blockSize >> blockResDivider;
            const uint32_t nextResIndex =
                numBlocks.x * numBlocks.y * (1U << (2U * blockResDivider));

#if defined(_WIN32)
            using OMPIndex = int32_t;
#else
            using OMPIndex = uint32_t;
#endif

#pragma omp parallel for schedule(dynamic)
            for (OMPIndex i = index; i < nextResIndex; ++i) {
                if (!keep_rendering)
                    continue;

                if (i % (blockSize * blockSize) == 0) {
                    new_sample_available.store(true, std::memory_order_relaxed);
                    partialSPPRendered += invNumBlock;
                }

                const uint32_t block = i % (numBlocks.x * numBlocks.y);
                const uint32_t inBlockSample = i / (numBlocks.x * numBlocks.y);
                const Pixel blockPixel = blockPos(blockSize, inBlockSample);
                const Pixel pixel{block % numBlocks.x * blockSize + blockPixel.x,
                                  block / numBlocks.x * blockSize + blockPixel.y};

                if (pixel.x >= resolution.x || pixel.y >= resolution.y)
                    continue;

                const Point2D normalizedScreenCoords =
                    ((Point2D{pixel} + sub_pixel) * invResolution - 0.5f) * 2.0f;
                const Ray ray = camera.generateRay(normalizedScreenCoords);

                Color color;
                switch (params.mode) {
                case RayTracerParameters::RenderMode::Depth:
                    color = depthIntegrator(scene, ray);
                    break;
                case RayTracerParameters::RenderMode::Position:
                    color = positionIntegrator(scene, ray);
                    break;
                case RayTracerParameters::RenderMode::Normal:
                    color = normalIntegrator(scene, ray);
                    break;
                case RayTracerParameters::RenderMode::Whitted:
                    color = whittedIntegrator(scene, ray);
                    break;
                case RayTracerParameters::RenderMode::Path:
                    color = pathIntegrator(scene, ray);
                    break;
                }

                if (sppRendered)
                    film.addPixelColor(pixel, color);
                else
                    film.addPixelColorUnweighted(pixel, color, pixelSize);
            }
            index = nextResIndex;
            new_sample_available.store(true, std::memory_order_relaxed);
        }
        ++sppRendered;
        partialSPPRendered = 0.0f;
    }
}
