#include <render/light.h>
#include <render/material.h>
#include <render/raytracer.h>
#include <render/scene.h>

Color Material::Diffuse::eval(float cosThetaI) const
{
    // TODO: evaluate the diffuse BRDF and cosine terms
    // on the backside, the material should be black.
    if (cosThetaI > 0.0f)
        return albedo * invPi;

    return {0.0f};
}

Vector3D Material::reflect(Vector3D v)
{
    // TODO: reflect the vector v given in the local space with normal (0, 0, 1)
    return {-v.x, -v.y, v.z};
}

Vector3D Material::Dielectric::refract(Vector3D omegaO) const
{
    // TODO: return the refracted ray
    const float etaT{omegaO.z >= 0.0f ? this->etaI : this->etaO};
    const float etaO{omegaO.z >= 0.0f ? this->etaO : this->etaI};

    const float eta = etaO / etaT;
    const float x = -omegaO.x * eta;
    const float y = -omegaO.y * eta;
    const float zSqr = 1.0f - x * x - y * y;

    if (zSqr <= 0.0f) // total internal reflection
        return {};

    const float z = std::sqrt(zSqr);
    return {x, y, (omegaO.z >= 0.0f) ? -z : z};
}

float Material::Dielectric::fresnel(float cosThetaO, float cosThetaT) const
{
    if (cosThetaT == 0.0f)
        return 1.0f;

    const float etaT{cosThetaO < 0.0f ? this->etaO : this->etaI};
    const float etaO{cosThetaO < 0.0f ? this->etaI : this->etaO};

    cosThetaO = std::abs(cosThetaO);
    cosThetaT = std::abs(cosThetaT);

    // TODO: compute the fresnel term for dielectrics
    const float R1 = (etaO * cosThetaO - etaT * cosThetaT) / (etaO * cosThetaO + etaT * cosThetaT);
    const float R2 = (etaT * cosThetaO - etaO * cosThetaT) / (etaT * cosThetaO + etaO * cosThetaT);
    return (R1 * R1 + R2 * R2) / 2.0f;
}

Normal3D ShadingIntersection::computeShadingNormal() const
{
    const TriangleIndices& face = instance->mesh->getFaces().at(triangleIndex);
    const std::vector<Normal3D>& normals = instance->mesh->getNormals();

    // TODO: interpolate the vertex normals to compute the shading normal
    //  Normal = (1 - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2;
    const BarycentricCoordinates bary{this->bary};
    const Normal3D normal =  normals[face.v1] * bary.lambda1() + normals[face.v2] * bary.lambda2 + normals[face.v3] * bary.lambda3;

    return normalize(instance->normalToWorld * normal);  // e) exercise was told not to normalize, but I'm not shure
//    return normal;
}

Color RayTracer::computeDirectLight(const Scene& scene, const ShadingIntersection& its)
{
    const Material::Diffuse& brdf = its.instance->material.diffuse();

    Color result{};
    // TODO: compute light from all point lights
//    result = {0.5f}; // "wrong" ambient light - so you can see something before implementing this

    const std::vector<PointLight> lights = scene.getLights();
    for (const PointLight& light: lights)
        if (!intersect(scene, light.shadowRay(its.point))) {
            const float cosThetaI = dot(its.shadingFrame.toLocal(light.pos), its.normal);
            result += light.Li(its.point) * brdf.eval(cosThetaI) * cosThetaI;
        }

    return result;
}
