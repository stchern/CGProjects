#version 330 core

uniform mat4 mlp;
uniform mat4 mvp;
uniform mat4 model;

in vec3 position;
in vec3 normal;
in vec2 texCoords;

out vec4 vsLSPosition;
out vec4 vsWSPosition;
out vec3 vsWSNormal;
flat out vec3 vsWSNormalFlat;
out vec2 vsTexCoords;


void main() {
    gl_Position = mvp * vec4(position, 1.0);
    vsLSPosition = mlp * vec4(position, 1.0);
    vsWSPosition = model * vec4(position, 1.0);
    vsWSNormal = transpose(inverse(mat3(model)))*normal;
    vsWSNormalFlat = vsWSNormal;
    vsTexCoords = texCoords;
}
