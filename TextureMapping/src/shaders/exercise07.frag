#version 330 core

uniform vec3 albedo;
uniform bool shadeFlat;
uniform vec3 cameraPos;
uniform vec3 pointLightPos;
uniform vec3 pointLightPower;

uniform sampler2D albedoTexture;
uniform sampler2D normalMap;
uniform sampler2D roughnessTexture;
uniform sampler2DShadow shadowMap;

in vec4 lsPosition;
in vec4 wsPosition;
in vec3 wsNormal;
flat in vec3 wsTangent;
flat in vec3 wsBitangent;
flat in vec3 wsNormalFlat;
in vec2 texCoords;
out vec3 color;

const float pi = 3.14159265359f;
const float epsilon = 0.001f;

uniform bool textured;
uniform bool ggx;
uniform bool useNormalMapping;
uniform bool useShadowMapping;

uniform float alpha;
uniform vec3 eta;
uniform vec3 k;

/// microfacet distribution
float D(float cosThetaM, float alpha) {
    if (cosThetaM <= 0.0f)
        return 0.0f;
    float cosThetaMSqr = cosThetaM*cosThetaM;
    float alphaSqr = alpha*alpha;
    float x = 1.0f+cosThetaMSqr*(alphaSqr-1.0f);
    return alphaSqr/(pi*x*x);
}
float G1(float cosThetaV, float alpha) {
    float cosThetaVSqr = cosThetaV*cosThetaV;
    float sinThetaVSqr = 1.0f-cosThetaVSqr;
    float tanThetaVSqr = sinThetaVSqr/cosThetaVSqr;
    return 2.0f/(1.0f+sqrt(1.0f+alpha*alpha*tanThetaVSqr));
}
/// shadowing-masking term
float G(float cosThetaI, float cosThetaO, float alpha) {
    return G1(cosThetaI, alpha)*G1(cosThetaO, alpha);
}
/// full fresnel term for conductors (we could use F(0) and Schlick's approximation instead)
vec3 F(float cosTheta) {
    if (cosTheta <= 0.0f)
        return vec3(0.0f);

    float cosThetaSqr = cosTheta * cosTheta;
    float sinThetaSqr = 1.0f - cosThetaSqr;
    vec3 etaSqr = eta * eta;
    vec3 kSqr = k * k;

    vec3 temp1 = etaSqr - kSqr - sinThetaSqr;
    vec3 aSqrPlusBSqr = sqrt(temp1 * temp1 + etaSqr * kSqr * 4.0f);
    vec3 aSqr = (aSqrPlusBSqr + temp1) * 0.5f;

    vec3 twoACosTheta = sqrt(aSqr) * (2.0f * cosTheta);
    vec3 Rs = (aSqrPlusBSqr + cosThetaSqr - twoACosTheta)
             / (aSqrPlusBSqr + cosThetaSqr + twoACosTheta);

    vec3 temp2 = twoACosTheta * sinThetaSqr;
    // the fraction is multiplied by cosThetaSqr/cosThetaSqr to avoid computing tanThetaSqr
    vec3 Rp = Rs * ( (aSqrPlusBSqr*cosThetaSqr + sinThetaSqr*sinThetaSqr - temp2)
                   / (aSqrPlusBSqr*cosThetaSqr + sinThetaSqr*sinThetaSqr + temp2));

    return (Rs + Rp) * 0.5f;
}

float srgb(float x) {
    return x < 0.0031308 ? 12.92 * x : (1.055 * pow(x, 1.0/2.4) - 0.055);
}

void main() {
    color = vec3(0.0f);

    if (gl_FrontFacing) {
        vec3 wsPos = wsPosition.xyz/wsPosition.w;
        vec3 wsDir = pointLightPos-wsPos;

        vec3 normal = normalize(shadeFlat ? wsNormalFlat : wsNormal);
        if (textured && useNormalMapping) {
            // backface test before normal mapping
            if (dot(normal, wsDir) <= 0.0f)
                return;

            vec3 s = wsTangent;
            vec3 t = wsBitangent;

            // TODO: apply normal (bump) mapping here to replace the normal
            normal = normal;
        }

        float dist = length(wsDir);
        vec3 omegaI = normalize(wsDir);
        float cosThetaI = dot(normal, omegaI);
        // backface test
        if (cosThetaI <= 0.0f)
            return;

        if (useShadowMapping) {
            // TODO: apply shadow mapping
            // lookup the correct location in the shadowMap
            // if you get black, simply return
            bool shaded = false;
            if (shaded) {
                return;
            }
        }

        // diffuse shading
        color = albedo;
        if (textured)
            color = texture(albedoTexture, texCoords).rgb;

        color *= cosThetaI/pi;

        if (ggx)
        {
            vec3 omegaO = normalize(cameraPos-wsPos);
            float cosThetaO = dot(omegaO, normal);

            if (cosThetaI*cosThetaO > 0.0) {
                vec3 halfway = normalize(omegaI+omegaO);
                float cosThetaM = dot(normal, halfway);
                float cosThetaIM = dot(halfway, omegaI);
                if (cosThetaIM > 0.0f) {
                    float roughness = alpha;
                    if (textured)
                        roughness = texture(roughnessTexture, texCoords).r;

                    vec3 fresnel = F(cosThetaIM);
                    float shadowing = G(cosThetaI, cosThetaO, roughness);
                    float microfacet = D(cosThetaM, roughness);
                    // cosThetaI cancels out
                    color += fresnel*(shadowing*microfacet/(4.0f*cosThetaO));
                }
            }
        }

        // multiply with Li
        color *= pointLightPower/(dist*dist*(4.0f*pi));

        // convert to sRGB
        color = vec3(srgb(color.r), srgb(color.g), srgb(color.b));
    }
}
