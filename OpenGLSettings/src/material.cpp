#include <render/light.h>
#include <render/material.h>
#include <render/raytracer.h>
#include <render/scene.h>

Color Material::Diffuse::eval(float cosThetaI) const
{
    if (cosThetaI > 0.0f)
        return albedo * invPi;

    return {0.0f};
}

Vector3D Material::reflect(Vector3D v) { return {-v.x, -v.y, v.z}; }

Vector3D Material::Dielectric::refract(Vector3D omegaO) const
{
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
    if (cosThetaT == 0.0f) // total internal reflection
        return 1.0f;

    const float etaO{cosThetaO >= 0.0f ? this->etaO : this->etaI};
    const float etaT{cosThetaO >= 0.0f ? this->etaI : this->etaO};

    cosThetaO = std::abs(cosThetaO);
    cosThetaT = std::abs(cosThetaT);

    const float Rs = (etaO * cosThetaO - etaT * cosThetaT) / (etaO * cosThetaO + etaT * cosThetaT);
    const float Rp = (etaT * cosThetaO - etaO * cosThetaT) / (etaT * cosThetaO + etaO * cosThetaT);

    return (Rs * Rs + Rp * Rp) * 0.5f;
}
