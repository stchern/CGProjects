#include <render/gl_shader.h>

#include <nanogui/opengl.h>

#include <render/scene.h>

#include <string>

using namespace nanogui;

using namespace std::string_literals;

static const std::string vertex_shader = R"(
#version 330

uniform mat4 mlp;
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

uniform vec3 albedo;
out vec3 color;

void main() {
    color = albedo;
}
)"s;

static const std::string vertex_shader_checkers = R"(
#version 330

uniform mat4 matrix_background;
in vec2 position;
out vec2 position_background;

void main() {
    vec4 p = vec4(position, 0.0, 1.0);
    gl_Position = p;
    position_background = (matrix_background * p).xy;
}
)"s;

static const std::string fragment_shader_checkers = R"(
#version 330

in vec2 position_background;
out vec3 frag_color;

void main() {
    vec2 frac = position_background - floor(position_background);
    float checkerboard = ((frac.x > .5) == (frac.y > .5)) ? 0.4 : 0.5;
    frag_color = vec3(checkerboard);
}
)"s;

// defined in exercise06.cpp
extern const std::string vertex_shader_debug;
// defined in exercise06.cpp
extern const std::string fragment_shader_debug;
// defined in exercise06.cpp
extern const std::string fragment_shader;
// defined in exercise06.cpp
extern const std::string vertex_shader_wobble;

static const Point2D fullScreenQuad[]{{-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f},
                                      {1.f, -1.f},  {1.f, 1.f},  {-1.f, 1.f}};

BackgroundShader::BackgroundShader()
    : backgroundShader{vertex_shader_checkers, fragment_shader_checkers}
{
    vertexBuffer.setBuffer<Point2D>("position", fullScreenQuad);
    backgroundShader.activate();
    backgroundShader.setVertexAttribute<Point2D>("position", vertexBuffer);
    backgroundShader.deactivate();
}

void BackgroundShader::draw(Point2D framebufferSize)
{
    const HomogeneousTransformation3D matrixBackground{
        Matrix3D::scale({framebufferSize.x / 40.f, framebufferSize.y / 40.f, 1.f}),
        {-framebufferSize.x / 40.f, -framebufferSize.y / 40.f, 0.f}};
    backgroundShader.activate();
    backgroundShader.setUniform("matrix_background", Matrix4D{matrixBackground});
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
    backgroundShader.deactivate();
}

BVHShader::BVHShader(const Instance& instance)
    : model{instance.toWorld}, bvhShader{vertex_shader_flat, fragment_shader_flat}
{
    const Mesh& mesh = instance.mesh;

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

        vertexBuffer.setBuffer<Point3D>("position", std::span<const Point3D>(bvhVertices));
    }
    {
        std::vector<LineIndices> bvhIndices;
        bvhIndices.reserve(numBVHNodes * 24);
        for (uint32_t i = 0; i < numBVHNodes; ++i) {
            // base
            bvhIndices.push_back({8 * i + 0, 8 * i + 1});
            bvhIndices.push_back({8 * i + 1, 8 * i + 3});
            bvhIndices.push_back({8 * i + 3, 8 * i + 2});
            bvhIndices.push_back({8 * i + 2, 8 * i + 0});

            // side
            bvhIndices.push_back({8 * i + 0, 8 * i + 4});
            bvhIndices.push_back({8 * i + 1, 8 * i + 5});
            bvhIndices.push_back({8 * i + 2, 8 * i + 6});
            bvhIndices.push_back({8 * i + 3, 8 * i + 7});

            // top
            bvhIndices.push_back({8 * i + 4, 8 * i + 5});
            bvhIndices.push_back({8 * i + 5, 8 * i + 7});
            bvhIndices.push_back({8 * i + 7, 8 * i + 6});
            bvhIndices.push_back({8 * i + 6, 8 * i + 4});
        }

        vertexBuffer.setBuffer<LineIndices>("lines", std::span<const LineIndices>(bvhIndices));
    }

    bvhShader.activate();
    bvhShader.setUniform("albedo",
                         ::Color{nanogui::Color{instance.material.albedo()}.contrasting_color()});
    bvhShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    bvhShader.deactivate();
}

void BVHShader::draw(uint32_t bvhLevel, const Matrix4D& vp)
{
    bvhShader.activate();
    bvhShader.setUniform("mvp", vp * model);

    const size_t displayBVHNodes =
        std::min(numBVHNodes, static_cast<size_t>(1UL << bvhLevel) - 1UL);
    vertexBuffer.drawLines("lines", 0, displayBVHNodes * 12);

    bvhShader.deactivate();
}

MeshShader::MeshShader(const Instance& instance)
    : model{instance.toWorld}, material{instance.material}, meshShader{vertex_shader,
                                                                       fragment_shader},
      wobbleShader{vertex_shader_wobble, fragment_shader}, debugShader{vertex_shader_debug,
                                                                       fragment_shader_debug}
{
    const Mesh& mesh = instance.mesh;

    vertexBuffer.setBuffer<Point3D>("position", std::span<const Point3D>(mesh.getVertices()));
    vertexBuffer.setBuffer<Normal3D>("normal", std::span<const Normal3D>(mesh.getNormals()));
    vertexBuffer.setBuffer<TriangleIndices>("triangles", std::span<const TriangleIndices>(mesh.getFaces()));

    numTriangles = mesh.getFaces().size();
    smoothGroups = mesh.getSmoothGroups();

    meshShader.activate();
    meshShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    meshShader.setVertexAttribute<Normal3D>("normal", vertexBuffer);
    meshShader.setUniform("albedo", instance.material.albedo());
    meshShader.deactivate();

    wobbleShader.activate();
    wobbleShader.setUniform("albedo", instance.material.albedo());
    wobbleShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    wobbleShader.setVertexAttribute<Normal3D>("normal", vertexBuffer);
    wobbleShader.deactivate();

    debugShader.activate();
    debugShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    debugShader.setVertexAttribute<Normal3D>("normal", vertexBuffer);
    debugShader.deactivate();
}

void MeshShader::draw(const Light::Point& light, const Matrix4D& vp)
{
    meshShader.activate();

    meshShader.setUniform("mvp", vp * model);
    meshShader.setUniform("model", model);
    meshShader.setUniform("point_light_pos", light.pos);
    meshShader.setUniform("point_light_power", light.power);

    // draw flat parts
    {
        meshShader.setUniform("shade_flat", true);
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                vertexBuffer.drawTriangles("triangles", pos, start);
            pos = end;
        }
        vertexBuffer.drawTriangles("triangles", pos, numTriangles);
    }
    // draw smooth parts
    {
        meshShader.setUniform("shade_flat", false);
        for (auto [start, end] : smoothGroups)
            vertexBuffer.drawTriangles("triangles", start, end);
    }

    meshShader.deactivate();
}

void MeshShader::drawWobble(const Light::Point& light, const Matrix4D& vp, float time)
{
    wobbleShader.activate();

    wobbleShader.setUniform("mvp", vp * model);
    wobbleShader.setUniform("model", model);
    wobbleShader.setUniform("point_light_pos", light.pos);
    wobbleShader.setUniform("point_light_power", light.power);
    wobbleShader.setUniform("time", time);

    // draw flat parts
    {
        wobbleShader.setUniform("shade_flat", true);
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                vertexBuffer.drawTriangles("triangles", pos, start);
            pos = end;
        }
        vertexBuffer.drawTriangles("triangles", pos, numTriangles);
    }
    // draw smooth parts
    {
        wobbleShader.setUniform("shade_flat", false);
        for (auto [start, end] : smoothGroups)
            vertexBuffer.drawTriangles("triangles", start, end);
    }

    wobbleShader.deactivate();
}

void MeshShader::drawDebug(const AABB& bounds, const Point3D& cameraPos, RenderMode mode,
                           const Matrix4D& vp)
{
    debugShader.activate();

    debugShader.setUniform("mvp", vp * model);
    debugShader.setUniform("model", model);
    debugShader.setUniform("mode", static_cast<int32_t>(mode));

    // extra parameters for depth and position modes
    if (mode == RenderMode::Depth) {
        AABB boundsWithCamera = bounds;
        boundsWithCamera.extend(cameraPos);
        debugShader.setUniform("reference", cameraPos);
        debugShader.setUniform("extents", Vector3D{boundsWithCamera.extents().maxComponent()});
    }
    else if (mode == RenderMode::Position) {
        debugShader.setUniform("reference", bounds.min);
        debugShader.setUniform("extents", bounds.extents());
    }
    else if (mode == RenderMode::Normal)
        ;
    else
        throw std::logic_error("invalid render mode for debug shader");

    // draw flat parts
    {
        debugShader.setUniform("shade_flat", true);
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                vertexBuffer.drawTriangles("triangles", pos, start);
            pos = end;
        }
        vertexBuffer.drawTriangles("triangles", pos, numTriangles);
    }
    // draw smooth parts
    {
        debugShader.setUniform("shade_flat", false);
        for (auto [start, end] : smoothGroups)
            vertexBuffer.drawTriangles("triangles", start, end);
    }

    debugShader.deactivate();
}
