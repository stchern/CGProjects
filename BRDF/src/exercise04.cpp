#include <render/light.h>
#include <render/material.h>
#include <render/raytracer.h>
#include <render/scene.h>

Color Material::Diffuse::eval(float cosThetaI) const
{
    // TODO: evaluate the diffuse BRDF and cosine terms
    // on the backside, the material should be black.

    return {};
}

Vector3D Material::reflect(Vector3D v)
{
    // TODO: reflect the vector v given in the local space with normal (0, 0, 1)
    return {};
}

Vector3D Material::Dielectric::refract(Vector3D omegaO) const
{
    // TODO: return the refracted ray
    return {};
}

float Material::Dielectric::fresnel(float cosThetaO, float cosThetaT) const
{
    if (cosThetaT == 0.0f)
        return {};

    const float etaT{cosThetaO < 0.0f ? this->etaO : this->etaI};
    const float etaO{cosThetaO < 0.0f ? this->etaI : this->etaO};

    cosThetaO = std::abs(cosThetaO);
    cosThetaT = std::abs(cosThetaT);

    // TODO: compute the fresnel term for dielectrics
    return {};
}

Normal3D ShadingIntersection::computeShadingNormal() const
{
    const TriangleIndices& face = instance->mesh->getFaces().at(triangleIndex);
    const std::vector<Normal3D>& normals = instance->mesh->getNormals();

    // TODO: interpolate the vertex normals to compute the shading normal
    Normal3D normal{this->normal}; // "flat" geometric normal - to be replaced

    return normalize(instance->normalToWorld * normal);
}

Color RayTracer::computeDirectLight(const Scene& scene, const ShadingIntersection& its)
{
    const Material::Diffuse& brdf = its.instance->material.diffuse();

    Color result{};

    // TODO: compute light from all point lights
    result = {0.5f}; // "wrong" ambient light - so you can see something before implementing this

    return result;
}
