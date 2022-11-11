#include "meshcanvas.h"

#include "camera_parameters.h"
#include <cmath>
#include <iostream>
#include <nanogui/opengl.h>

using namespace nanogui;

MeshCanvas::MeshCanvas(Widget* parent) : Canvas{parent}
{
    static const std::string vertex_shader = R"(
#version 330

uniform mat4 mvp;
uniform mat4 mv;

in vec3 position;
in vec3 normal;
out vec3 ws_normal;
flat out vec3 ws_normal_flat;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    ws_normal = transpose(inverse(mat3(mv)))*normal;
    ws_normal_flat = ws_normal;
}
)";
    static const std::string fragment_shader = R"(
#version 330

uniform vec4 base_color;
uniform bool shade_flat;
uniform bool shade_normal;

in vec3 ws_normal;
flat in vec3 ws_normal_flat;
out vec4 color;

void main() {
    vec3 normal = normalize(shade_flat ? ws_normal_flat : ws_normal);
    if (shade_normal)
        color = vec4(normal*0.5+vec3(0.5), 1.0);
    else
        color = base_color*normal.z;
}
)";
    shader = new Shader(render_pass(), "mesh_shader", vertex_shader, fragment_shader);

    // coordinate axes
    axesShader = new Shader(render_pass(), "coord_shader", vertex_shader, fragment_shader);
    m_coordMesh.loadOBJ("../meshes/Axis.obj");
    axesShader->set_buffer("indices", VariableType::UInt32, {m_coordMesh.getFaces().size() * 3},
                           m_coordMesh.getFaces().data());
    axesShader->set_buffer("position", VariableType::Float32, {m_coordMesh.getVertices().size(), 3},
                           m_coordMesh.getVertices().data());
    axesShader->set_buffer("normal", VariableType::Float32, {m_coordMesh.getNormals().size(), 3},
                           m_coordMesh.getNormals().data());

    static const std::string vertex_shader_flat = R"(
#version 330

uniform mat4 mvp;
in vec3 position;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
}
)";
    static const std::string fragment_shader_flat = R"(
#version 330

uniform vec4 base_color;
out vec4 color;

void main() {
    color = base_color;
}
)";

    bvhShader = new Shader(render_pass(), "bvh_shader", vertex_shader_flat, fragment_shader_flat);
}

void MeshCanvas::uploadMesh(const Mesh& mesh)
{
    shader->set_buffer("indices", VariableType::UInt32, {mesh.getFaces().size() * 3},
                       mesh.getFaces().data());
    shader->set_buffer("position", VariableType::Float32, {mesh.getVertices().size(), 3},
                       mesh.getVertices().data());
    shader->set_buffer("normal", VariableType::Float32, {mesh.getNormals().size(), 3},
                       mesh.getNormals().data());

    numTriangles = mesh.getFaces().size();
    aabb = mesh.getBounds();
    smoothGroups = mesh.getSmoothGroups();

    const BVH& bvh = mesh.getBVH();
    const auto& bvhNodes = bvh.getNodes();
    numBVHNodes = bvhNodes.size();
    {
        std::vector<Point3D> bvhVertices;
        bvhVertices.reserve(numBVHNodes * 8);
        for (auto& node : bvhNodes) {
            bvhVertices.emplace_back(node.bounds.min.x, node.bounds.min.y, node.bounds.min.z);
            bvhVertices.emplace_back(node.bounds.min.x, node.bounds.min.y, node.bounds.max.z);
            bvhVertices.emplace_back(node.bounds.min.x, node.bounds.max.y, node.bounds.min.z);
            bvhVertices.emplace_back(node.bounds.min.x, node.bounds.max.y, node.bounds.max.z);
            bvhVertices.emplace_back(node.bounds.max.x, node.bounds.min.y, node.bounds.min.z);
            bvhVertices.emplace_back(node.bounds.max.x, node.bounds.min.y, node.bounds.max.z);
            bvhVertices.emplace_back(node.bounds.max.x, node.bounds.max.y, node.bounds.min.z);
            bvhVertices.emplace_back(node.bounds.max.x, node.bounds.max.y, node.bounds.max.z);
        }
        bvhShader->set_buffer("position", VariableType::Float32, {bvhVertices.size(), 3},
                              bvhVertices.data());
    }
    {
        std::vector<uint32_t> bvhIndices;
        bvhIndices.reserve(numBVHNodes * 24);
        for (uint32_t i = 0; i < numBVHNodes; ++i) {
            // base
            bvhIndices.push_back(8 * i + 0);
            bvhIndices.push_back(8 * i + 1);
            bvhIndices.push_back(8 * i + 1);
            bvhIndices.push_back(8 * i + 3);
            bvhIndices.push_back(8 * i + 3);
            bvhIndices.push_back(8 * i + 2);
            bvhIndices.push_back(8 * i + 2);
            bvhIndices.push_back(8 * i + 0);

            // side
            bvhIndices.push_back(8 * i + 0);
            bvhIndices.push_back(8 * i + 4);
            bvhIndices.push_back(8 * i + 1);
            bvhIndices.push_back(8 * i + 5);
            bvhIndices.push_back(8 * i + 2);
            bvhIndices.push_back(8 * i + 6);
            bvhIndices.push_back(8 * i + 3);
            bvhIndices.push_back(8 * i + 7);

            // top
            bvhIndices.push_back(8 * i + 4);
            bvhIndices.push_back(8 * i + 5);
            bvhIndices.push_back(8 * i + 5);
            bvhIndices.push_back(8 * i + 7);
            bvhIndices.push_back(8 * i + 7);
            bvhIndices.push_back(8 * i + 6);
            bvhIndices.push_back(8 * i + 6);
            bvhIndices.push_back(8 * i + 4);
        }
        bvhShader->set_buffer("indices", VariableType::UInt32, {bvhIndices.size()},
                              bvhIndices.data());
    }
}

void MeshCanvas::draw_contents()
{
    if (!numTriangles)
        return;

    const MeshDisplayParameters& displayParams =
        meshDisplayControls ? meshDisplayControls->getDisplayParameters() : MeshDisplayParameters{};

    Matrix4f model = Matrix4f::translate({0.0f, 0.0f, 0.0f});

    const CameraParameters& cameraParams =
        cameraControls ? cameraControls->getCameraParameters(m_size) : CameraParameters{};

    Matrix4f view = Matrix4f::look_at(cameraParams.pos, cameraParams.target, cameraParams.up);

    if (displayParams.autoScale) {
        const float scale = (displayParams.autoCenter)
                              ? 2.0f / aabb.extents().maxComponent()
                              : 1.0f / max(abs(aabb.min), abs(aabb.max)).maxComponent();
        view = view * Matrix4f::scale(Vector3f{scale});
    }
    if (displayParams.autoCenter)
        view = view * Matrix4f::translate(-aabb.center());

    Matrix4f proj;
    if (cameraParams.type == CameraParameters::CameraType::Perspective)
        proj = Matrix4f::perspective(cameraParams.perspective.fov * degToRad, 0.1f, 20.f,
                                     cameraParams.aspect);
    else
        proj = Matrix4f::ortho(cameraParams.orthographic.left, cameraParams.orthographic.right,
                               cameraParams.orthographic.bottom, cameraParams.orthographic.top,
                               0.1f, 20.f);

    const Matrix4f mvp = proj * view * model;

    set_background_color(displayParams.bgColor);

    if (displayParams.showAxes) {
        axesShader->set_uniform("mvp", proj * view);
        axesShader->set_uniform("mv", view);

        axesShader->set_uniform("shade_flat", true);
        axesShader->set_uniform("shade_normal", displayParams.normals);
        axesShader->set_uniform("base_color", Color{1.0f} - background_color());

        axesShader->begin();
        axesShader->draw_array(Shader::PrimitiveType::Triangle, 0,
                               m_coordMesh.getFaces().size() * 3, true);
        axesShader->end();
    }
    if (displayParams.bvhLevel) {
        bvhShader->set_uniform("mvp", mvp);
        bvhShader->set_uniform("base_color", Color{1.0f} - background_color());

        size_t displayBVHNodes = std::min(numBVHNodes, (1UL << displayParams.bvhLevel) - 1);

        bvhShader->begin();
        bvhShader->draw_array(Shader::PrimitiveType::Line, 0, displayBVHNodes * 24, true);
        bvhShader->end();
    }

    shader->set_uniform("mvp", mvp);
    shader->set_uniform("mv", model);

    shader->set_uniform("base_color", displayParams.fgColor);
    shader->set_uniform("shade_flat", true);
    shader->set_uniform("shade_normal", displayParams.normals);
    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);

    if (displayParams.wireframe) {
        render_pass()->set_cull_mode(nanogui::RenderPass::CullMode::Disabled);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
        render_pass()->set_cull_mode(nanogui::RenderPass::CullMode::Back);

    shader->begin();

    // draw flat parts
    {
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                shader->draw_array(Shader::PrimitiveType::Triangle, pos * 3, (start - pos) * 3,
                                   true);
            pos = end;
        }
        shader->draw_array(Shader::PrimitiveType::Triangle, pos * 3, (numTriangles - pos) * 3,
                           true);
    }
    // draw smooth parts
    if (smoothGroups.size()) {
        shader->end();
        shader->set_uniform("shade_flat", false);
        shader->begin();
        for (auto [start, end] : smoothGroups)
            shader->draw_array(Shader::PrimitiveType::Triangle, start * 3, (end - start) * 3, true);
    }

    shader->end();

    if (displayParams.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
