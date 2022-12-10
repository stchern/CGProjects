#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include <common/constants.h>
#include <geometry/point3d.h>

#include <variant>

struct OrthonormalSystem {
    Vector3D s{1.0f, 0.0f, 0.0f}, t{0.0f, 1.0f, 0.0f};
    Normal3D n{0.0f, 0.0f, 1.0f};

    OrthonormalSystem() = default;

    /**
     * @brief OrthonormalSystem
     * @param n normal vector (must be normalized!)
     */
    OrthonormalSystem(Normal3D n)
        : OrthonormalSystem{(std::abs(n.x) > std::abs(n.y)) ? normalize({n.z, 0.0f, -n.x})
                                                            : normalize({0.0f, n.z, -n.y}),
                            n}
    {
    }

    Vector3D toLocal(Vector3D v) const { return {dot(v, s), dot(v, t), dot(v, n)}; }
    Vector3D toWorld(Vector3D v) const { return s * v.x + t * v.y + n * v.z; }

private:
    OrthonormalSystem(Vector3D t, Normal3D n) : s{cross(t, n)}, t{t}, n{n} {}
};

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
            const float tanThetaSqr = sinThetaSqr / cosThetaSqr;
            const Color etaSqr = eta * eta;
            const Color kSqr = k * k;

            const Color temp1 = etaSqr - kSqr - sinThetaSqr;
            const Color aSqrPlusBSqr = sqrt(temp1 * temp1 + etaSqr * kSqr * 4.0f);
            const Color aSqr = (aSqrPlusBSqr + temp1) * 0.5f;
            // const Color bSqr = (aSqrPlusBSqr-c)*0.5f;
            const Color a = sqrt(aSqr);

            const Color temp2 = a * (2.0f * cosTheta) + cosThetaSqr;
            const Color Rs = (aSqrPlusBSqr - temp2) / (aSqrPlusBSqr + temp2);

            const Color temp3 = a * (2.0f * cosTheta * tanThetaSqr) + sinThetaSqr * tanThetaSqr;
            const Color Rp = Rs * (aSqrPlusBSqr - temp3) / (aSqrPlusBSqr + temp3);

            return (Rs + Rp) * 0.5f;
        }
    };
    struct Dielectric {
        float etaI; // internal index of refraction
        float etaO; // external index of refraction

        Vector3D refract(Vector3D omegaO) const;
        float fresnel(float cosThetaO, float cosThetaT) const;
    };

    using Parameters = std::variant<Diffuse, Mirror, Conductor, Dielectric>;

    Parameters params{};

    // check what type of material this is

    bool isDiffuse() const { return std::holds_alternative<Diffuse>(params); }
    bool isMirror() const { return std::holds_alternative<Mirror>(params); }
    bool isDielectric() const { return std::holds_alternative<Dielectric>(params); }
    bool isGlass() const { return isDielectric(); }
    bool isConductor() const { return std::holds_alternative<Conductor>(params); }

    // get a specific material (make sure it is of that type first!)

    Diffuse diffuse() const { return std::get<Diffuse>(params); }
    Mirror mirror() const { return std::get<Mirror>(params); }
    Dielectric dielectric() const { return std::get<Dielectric>(params); }
    Conductor conductor() const { return std::get<Conductor>(params); }
};

#endif // MATERIAL_H
