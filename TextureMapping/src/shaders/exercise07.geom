#version 410 core

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

uniform mat4 model;
uniform mat4 mvp;
uniform sampler2D displacementMap;
uniform float displacementScale;
uniform bool shadeFlat;
uniform bool textured;
uniform bool useDisplacementMapping;

in vec4 vsLSPosition[];
in vec4 vsWSPosition[];
in vec3 vsWSNormal[];
flat in vec3 vsWSNormalFlat[];
in vec2 vsTexCoords[];

out vec4 lsPosition;
out vec4 wsPosition;
out vec3 wsNormal;
flat out vec3 wsTangent;
flat out vec3 wsBitangent;
flat out vec3 wsNormalFlat;
out vec2 texCoords;

void main()
{
    // TODO: use vsWSPosition and vsTexCoords to compute the tangent and bi-tangent vectors
    float denominator = 1.0f / ( (vsTexCoords[1][0] - vsTexCoords[0][0]) * (vsTexCoords[2][1] - vsTexCoords[0][1]) -
                                (vsTexCoords[1][1] - vsTexCoords[0][1]) * (vsTexCoords[2][0] - vsTexCoords[0][0]));
    vec3 s = vec3((vsWSPosition[1] - vsWSPosition[0]) * (vsTexCoords[2][1] - vsTexCoords[0][1]) -
                  (vsWSPosition[2] - vsWSPosition[0])*(vsTexCoords[1][1] - vsTexCoords[0][1])) * denominator;
    vec3 t = vec3((vsWSPosition[2] - vsWSPosition[0]) * (vsTexCoords[1][0] - vsTexCoords[0][0]) -
                    (vsWSPosition[1] - vsWSPosition[0])*(vsTexCoords[2][0] - vsTexCoords[0][0])) * denominator;

    // now, compute the per-vertex data

    for (int vertex = 0; vertex < 3; vertex++)
    {
        lsPosition    = vsLSPosition[vertex];
        wsPosition    = vsWSPosition[vertex];
        wsNormal      = vsWSNormal[vertex];
        wsTangent     = s;
        wsBitangent   = t;
        wsNormalFlat  = vsWSNormalFlat[vertex];
        texCoords = vsTexCoords[vertex];
        gl_Position = gl_in[vertex].gl_Position;

        if (textured && useDisplacementMapping) {
            vec3 normal = shadeFlat ? wsNormalFlat : wsNormal;
            mat4 vp = mvp * inverse(model);

            // TODO: overwrite the gl_Position with the displaced position
            // when applying the model view projection matrix, make sure to set w=1 in your vec4
            float displacementSample = texture(displacementMap, texCoords).r - 0.5f;
            wsPosition.xyz += displacementSample * displacementScale * normal;
            gl_Position = vp * vec4(wsPosition.xyz, 1.0);
        }

        EmitVertex();
    }

    EndPrimitive();
}
