#include <render/mesh_shader.h>

#include <nanogui/opengl.h>

#include <render/scene.h>

#include <string>

using namespace nanogui;

using namespace std::string_literals;

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
)"s;
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
)"s;

static const std::string vertex_shader_flat = R"(
#version 330

uniform mat4 mvp;
in vec3 position;

void main() {
gl_Position = mvp * vec4(position, 1.0);
}
)"s;
static const std::string fragment_shader_flat = R"(
#version 330

uniform vec4 base_color;
out vec4 color;

void main() {
color = base_color;
}
)"s;

MeshShader::MeshShader(const Instance& instance, RenderPass* render_pass) : model{instance.toWorld}
{
    const Mesh& mesh = instance.mesh;

    meshShader = new Shader(render_pass, "mesh_shader", vertex_shader, fragment_shader);
    bvhShader = new Shader(render_pass, "bvh_shader", vertex_shader_flat, fragment_shader_flat);

    meshShader->set_buffer("indices", VariableType::UInt32, {mesh.getFaces().size() * 3},
                           mesh.getFaces().data());
    meshShader->set_buffer("position", VariableType::Float32, {mesh.getVertices().size(), 3},
                           mesh.getVertices().data());
    meshShader->set_buffer("normal", VariableType::Float32, {mesh.getNormals().size(), 3},
                           mesh.getNormals().data());

    numTriangles = mesh.getFaces().size();
    smoothGroups = mesh.getSmoothGroups();

    nanogui::Color albedo =
        instance.material.isDiffuse()
            ? nanogui::Color{std::get<Material::Diffuse>(instance.material.params).albedo}
            : nanogui::Color{165 / 255.0f, 30 / 255.0f, 55 / 255.0f, 1.0f};

    meshShader->set_uniform("base_color", albedo);
    bvhShader->set_uniform("base_color", albedo.contrasting_color());

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

void MeshShader::draw(const MeshDisplayParameters& displayParams, const Matrix4f& vp)
{
    if (!numTriangles)
        return;

    if (displayParams.bvhLevel) {
        bvhShader->set_uniform("mvp", vp * model);

        size_t displayBVHNodes = std::min(numBVHNodes, static_cast<size_t>(1UL << displayParams.bvhLevel) - 1UL);

        bvhShader->begin();
        bvhShader->draw_array(Shader::PrimitiveType::Line, 0, displayBVHNodes * 24, true);
        bvhShader->end();
    }

    meshShader->set_uniform("mvp", vp * model);
    meshShader->set_uniform("mv", model);

    meshShader->set_uniform("shade_flat", true);
    meshShader->set_uniform("shade_normal", displayParams.normals);
    glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);

    meshShader->begin();

    // draw flat parts
    {
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                meshShader->draw_array(Shader::PrimitiveType::Triangle, pos * 3, (start - pos) * 3,
                                       true);
            pos = end;
        }
        meshShader->draw_array(Shader::PrimitiveType::Triangle, pos * 3, (numTriangles - pos) * 3,
                               true);
    }
    // draw smooth parts
    if (smoothGroups.size()) {
        meshShader->end();
        meshShader->set_uniform("shade_flat", false);
        meshShader->begin();
        for (auto [start, end] : smoothGroups)
            meshShader->draw_array(Shader::PrimitiveType::Triangle, start * 3, (end - start) * 3,
                                   true);
    }

    meshShader->end();
}
