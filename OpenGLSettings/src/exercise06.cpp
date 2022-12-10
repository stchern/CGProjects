#include <render/gl_shader.h>
#include <render/gl_utils.h>

#include <string>
#include <iostream>

using namespace std::string_literals;

const static std::string my_little_vertex_shader = R"(
#version 330

// TODO: add input/output parameters
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
uniform float scale;

out vec4 vertexColor;

void main() {
    // TODO: fill me in
    gl_Position = scale * vec4(inPos, 1.0);
    vertexColor = vec4(inColor, 1.0);
}
)"s;

const static std::string my_little_fragment_shader = R"(
#version 330

// TODO: add input/output parameters
out vec4 FragColor;

in vec4 vertexColor;

void main() {
    // TODO: fill me in
    FragColor = vertexColor;
}
)"s;

extern const std::string vertex_shader_debug = R"(
#version 330

uniform mat4 mvp;
uniform mat4 model;

in vec3 position;
in vec3 normal;
out vec4 ws_position;
out vec3 ws_normal;
flat out vec3 ws_normal_flat;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    ws_position = model * vec4(position, 1.0);
    ws_normal = transpose(inverse(mat3(model)))*normal;
    ws_normal_flat = ws_normal;
}
)"s;

extern const std::string fragment_shader_debug = R"(
#version 330

uniform int mode; // 0: depth, 1: position, 2: normal
uniform vec3 reference; // depth: camera pos / position: minimum scene point
uniform vec3 extents; // depth/position: scene extents (including/excluding camera)

uniform bool shade_flat;

in vec4 ws_position;
in vec3 ws_normal;
flat in vec3 ws_normal_flat;
out vec3 color;

void main() {
    if (mode == 0) { // depth
        vec3 cameraPos = reference;
        vec3 maxDepth = extents;

        // TODO: compute the depth from the world space position and the camera position
        // TODO: normalize using the maximum depth
        float distance = length(cameraPos - ws_position.xyz);
        color = vec3(distance) / maxDepth;
    }
    else if (mode == 1) { // position
        vec3 aabbMin = reference;
        vec3 aabbExtents = extents;

        // TODO: set the color to the normalized (in [0,1]^3) world space position
        // TODO: use the minimum and the extents of the scene bounds for normalization
        color = vec3(ws_position.xyz - aabbMin) / aabbExtents;
    }
    else if (mode == 2) { // normal
        vec3 normal = normalize(shade_flat ? ws_normal_flat : ws_normal);

        // TODO: set the color to the normalized (in [0,1]^3) world space normal
        color = vec3(normal * 0.5f + 0.5f);
    }
    else {
        color = vec3(0.0);
    }
}
)"s;

extern const std::string vertex_shader_wobble = R"(
#version 330

uniform mat4 mlp;
uniform mat4 mvp;
uniform mat4 model;

uniform float time;

in vec3 position;
in vec3 normal;
out vec4 ws_position;
out vec3 ws_normal;
flat out vec3 ws_normal_flat;

void main() {
    // TODO: apply the "wobble" transformation as described on the exercise sheet

  //float constant = 0.25 * sin(5 * (time + position.y));
  //vec3 newPosition = vec3(1 + constant * position.x, position.y, 1 + constant * position.z);
    gl_Position = mvp * vec4(position, 1.0);
    ws_position = model * vec4(position, 1.0);
    ws_normal = transpose(inverse(mat3(model)))*normal;
    ws_normal_flat = ws_normal;
}
)"s;

// matching vertex shader is implemented in gl_shader.cpp
extern const std::string fragment_shader = R"(
#version 330

uniform vec3 albedo;
uniform bool shade_flat;
uniform vec3 point_light_pos;
uniform vec3 point_light_power;

in vec4 ws_position;
in vec3 ws_normal;
flat in vec3 ws_normal_flat;
out vec3 color;

const float pi = 3.14159265359f;

float srgb(float x) {
    return x < 0.0031308 ? 12.92 * x : (1.055 * pow(x, 1.0/2.4) - 0.055);
}

void main() {
    vec3 normal = normalize(shade_flat ? ws_normal_flat : ws_normal);
    if (gl_FrontFacing) {
        // TODO: compute the diffuse shading
        // use the world space position, world space normal,
        // point light position, and point light power
        // to compute the shading terms

        vec3 ws_pos = ws_position.xyz/ws_position.w;

        // simple shading using ambient light and the world-space normal -- remove this
        color = albedo*(0.25+0.75*max(0, normal.z));

        // convert to sRGB
        color = vec3(srgb(color.r), srgb(color.g), srgb(color.b));
    }
    else {
        color = vec3(0.0);
    }
}
)"s;

MyLittleShader::MyLittleShader()
{
    // TODO: setup your OpenGL shader program, vertex array, and buffers

    GLchar const* shaders[] = {my_little_vertex_shader.c_str(), my_little_fragment_shader.c_str()};
    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &shaders[0], NULL);
    glCompileShader(vertexShader);

//     check compilation errors for vertex shader
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout<< "PROBLEM WITH VERTEX SHADER" << std::endl;
    }

    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &shaders[1], NULL);
    glCompileShader(fragmentShader);

    // check compilation errors for fragment shader
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout<< "PROBLEM WITH FRAGMENT SHADER" << std::endl;
        }

    // binding shaders
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // check bindings errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout<< "PROBLEM WITH SHADER PROGRAM" << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    const float vertices[] = {
        // first triangle
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,

        // second triangle
        0.5f, -0.5f, 0.0f,
        0.5f,  0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f
    };

    const float colors[] = {
          1.0f, 1.0f, 1.0f,
          1.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f,

          1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f,
          0.0f, 1.0f, 0.0f,
    };

    glGenBuffers(1, &buffers[0]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &buffers[1]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    CHECK_GL();
}

MyLittleShader::~MyLittleShader()
{
    // TODO: cleanup your OpenGL shader program, vertex array, and buffers
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &buffers[0]);
    glDeleteBuffers(1, &buffers[1]);
    CHECK_GL();
}

void MyLittleShader::draw(float time)
{
    // checking if something went wrong somewhere else
    CHECK_GL();

    // TODO: draw your OpenGL vertex array
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);

    float timeValue = glfwGetTime();
    float scaleValue = 0.25f * sin(timeValue) + 0.5f;
    int scaleParam = glGetUniformLocation(program, "scale");
    glUniform1f(scaleParam, scaleValue);

    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
    CHECK_GL();
}
