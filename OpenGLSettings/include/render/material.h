#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include <common/constants.h>
#include <geometry/frame.h>
#include <geometry/point3d.h>

#include <variant>

struct Material {
    static Vector3D reflect(Vector3D v);

    struct Diffuse {
        Color albedo;

        Color eval(float cosThetaI) const;
    };
    struct Mirror {};
    struct Conductor {
        Color eta;
        Color k;

        Color fresnel(float cosTheta) const
        {
            const float cosThetaSqr = cosTheta * cosTheta;
            const float sinThetaSqr = 1.0f - cosThetaSqr;
            const Color etaSqr = eta * eta;
            const Color kSqr = k * k;

            const Color temp1 = etaSqr - kSqr - sinThetaSqr;
            const Color aSqrPlusBSqr = sqrt(temp1 * temp1 + etaSqr * kSqr * 4.0f);
            const Color aSqr = (aSqrPlusBSqr + temp1) * 0.5f;

            const Color twoACosTheta = sqrt(aSqr) * (2.0f * cosTheta);
            const Color Rs = (aSqrPlusBSqr + cosThetaSqr - twoACosTheta)
                           / (aSqrPlusBSqr + cosThetaSqr + twoACosTheta);

            const Color temp2 = twoACosTheta * sinThetaSqr;
            // the fraction is multiplied by cosThetaSqr/cosThetaSqr to avoid computing tanThetaSqr
            const Color Rp = Rs
                           * ((aSqrPlusBSqr * cosThetaSqr + sinThetaSqr * sinThetaSqr - temp2)
                              / (aSqrPlusBSqr * cosThetaSqr + sinThetaSqr * sinThetaSqr + temp2));

            return (Rs + Rp) * 0.5f;
        }
    };
    struct Dielectric {
        /// internal index of refraction
        float etaI;
        /// external index of refraction
        float etaO;

        Vector3D refract(Vector3D omegaO) const;
        float fresnel(float cosThetaO, float cosThetaT) const;
    };
    struct GGX {
        /// roughness (0 = perfectly specular, 1 = diffuse)
        float alpha;

        /// microfacet distribution
        float D(float cosThetaM) const
        {
            if (cosThetaM <= 0.0f)
                return 0.0f;
            const float cosThetaMSqr = cosThetaM * cosThetaM;
            const float sinThetaMSqr = 1.0f - cosThetaMSqr;
            const float tanThetaMSqr = sinThetaMSqr / cosThetaMSqr;
            const float alphaSqr = alpha * alpha;
            const float x = cosThetaMSqr * (alphaSqr + tanThetaMSqr);
            return alphaSqr / (pi * x * x);
        }
        float G1(float cosThetaVM) const
        {
            if (cosThetaVM <= 0.0f)
                return 0.0f;
            const float cosThetaVMSqr = cosThetaVM * cosThetaVM;
            const float sinThetaVMSqr = 1.0f - cosThetaVMSqr;
            const float tanThetaVMSqr = sinThetaVMSqr / cosThetaVMSqr;
            return 2.0f / (1.0f + std::sqrt(1.0f + alpha * alpha * tanThetaVMSqr));
        }
        /// shadowing-masking term
        float G(float cosThetaIM, float cosThetaOM) const
        {
            return G1(cosThetaIM) * G1(cosThetaOM);
        }
    };
    struct RoughConductor {
        Conductor conductor{};
        GGX microfacetDistribution{};

        Color eval(Vector3D omegaO, Vector3D omegaI) const
        {
            if (omegaO.z * omegaI.z <= 0.0f)
                return {0.0f};

            const Normal3D halfway = normalize(omegaO + omegaI);

            const Color F = conductor.fresnel(dot(halfway, omegaI));
            const float G = microfacetDistribution.G(dot(omegaI, halfway), dot(omegaO, halfway));
            const float D = microfacetDistribution.D(halfway.z);

            return F * (G * D / (4.0f * omegaI.z * omegaO.z));
        }
    };

    using Parameters = std::variant<Diffuse, Mirror, Conductor, Dielectric, RoughConductor>;

    Parameters params{};
    Color emittedRadiance{};

    // check what type of material this is

    bool isDiffuse() const { return std::holds_alternative<Diffuse>(params); }
    bool isMirror() const { return std::holds_alternative<Mirror>(params); }
    bool isDielectric() const { return std::holds_alternative<Dielectric>(params); }
    bool isGlass() const { return isDielectric(); }
    bool isConductor() const { return std::holds_alternative<Conductor>(params); }
    bool isRoughConductor() const { return std::holds_alternative<RoughConductor>(params); }

    bool isEmitter() const { return !emittedRadiance.isBlack(); }

    // get a specific material (make sure it is of that type first!)

    Diffuse diffuse() const { return std::get<Diffuse>(params); }
    Mirror mirror() const { return std::get<Mirror>(params); }
    Dielectric dielectric() const { return std::get<Dielectric>(params); }
    Conductor conductor() const { return std::get<Conductor>(params); }
    RoughConductor roughConductor() const { return std::get<RoughConductor>(params); }

    Color eval(Vector3D omegaO, Vector3D omegaI) const
    {
        if (isDiffuse()) {
            return diffuse().eval(omegaI.z);
        }
        else if (isRoughConductor()) {
            return roughConductor().eval(omegaO, omegaI);
        }
        return {0.0f};
    }

    /// get some color representing this material
    Color albedo() const
    {
        if (isDiffuse())
            return diffuse().albedo;
        else if (isConductor())
            return conductor().fresnel(invPi);
        else if (isDielectric())
            return dielectric().fresnel(0.0f, invPi);
        else if (isRoughConductor())
            return roughConductor().eval({0.0f, 0.0f, 1.0f}, normalize({0.0f, invPi, 1.0f}));
        else
            return Color{165 / 255.0f, 30 / 255.0f, 55 / 255.0f, 1.0f};
    }
};

#endif // MATERIAL_H
