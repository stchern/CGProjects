#include <render/gl_shader.h>

#include <nanogui/opengl.h>

#include <render/scene.h>

#include <fstream>
#include <sstream>
#include <string>

using namespace std::string_literals;

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

const static std::string my_little_vertex_shader = R"(
#version 330

uniform float scale;

in vec2 position;
in vec4 color;
out vec3 vsColor;

void main() {
    gl_Position = vec4(position*scale, 0.0, 1.0);
    vsColor = color.rgb;
}
)"s;
const static std::string my_little_fragment_shader = R"(
#version 330

in vec3 vsColor;
out vec3 fragColor;

void main() {
    fragColor = vsColor;
}
)"s;

static const std::string vertex_shader_debug = R"(
#version 330

uniform mat4 mvp;
uniform mat4 model;

in vec3 position;
in vec3 normal;
in vec2 texCoords;

out vec4 wsPosition;
out vec3 wsNormal;
flat out vec3 wsNormalFlat;
out vec2 fragTexCoords;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    wsPosition = model * vec4(position, 1.0);
    wsNormal = transpose(inverse(mat3(model)))*normal;
    wsNormalFlat = wsNormal;
    fragTexCoords = texCoords;
}
)"s;

static const std::string fragment_shader_debug = R"(
#version 330

uniform int mode; // 0: depth, 1: position, 2: normal
uniform vec3 reference; // depth: camera pos / position: minimum scene point
uniform vec3 extents; // depth/position: scene extents (including/excluding camera)

uniform bool shadeFlat;

in vec4 wsPosition;
in vec3 wsNormal;
flat in vec3 wsNormalFlat;
in vec2 fragTexCoords;

out vec3 color;

void main() {
    if (mode == 0) { // depth
        vec3 cameraPos = reference;
        vec3 maxDepth = extents;

        color = length(wsPosition.xyz-cameraPos)/maxDepth;
    }
    else if (mode == 1) { // position
        vec3 aabbMin = reference;
        vec3 aabbExtents = extents;

        color = (wsPosition.xyz-aabbMin)/aabbExtents;
    }
    else if (mode == 2) { // normal
        vec3 normal = normalize(shadeFlat ? wsNormalFlat : wsNormal);

        color = normal*0.5+vec3(0.5);
    }
    else if (mode == 3) { // uv
        color = vec3(fragTexCoords, 0.0f);
    }
    else {
        color = vec3(0.0);
    }
}
)"s;

static const std::string vertex_shader_wobble = R"(
#version 330

uniform mat4 mlp;
uniform mat4 mvp;
uniform mat4 model;

uniform float time;

in vec3 position;
in vec3 normal;
out vec4 wsPosition;
out vec3 wsNormal;
flat out vec3 wsNormalFlat;

void main() {
    vec3 pos;
    pos.x = (1.0+0.25*sin(5.0*(time+position.y)))*position.x;
    pos.y = position.y;
    pos.z = (1.0+0.25*sin(5.0*(time+position.y)))*position.z;

    gl_Position = mvp * vec4(pos, 1.0);
    wsPosition = model * vec4(pos, 1.0);
    wsNormal = transpose(inverse(mat3(model)))*normal;
    wsNormalFlat = wsNormal;
}
)"s;

static const std::string fragment_shader_wobble = R"(
#version 330

uniform vec3 albedo;
uniform bool shadeFlat;
uniform vec3 pointLightPos;
uniform vec3 pointLightPower;

in vec4 wsPosition;
in vec3 wsNormal;
flat in vec3 wsNormalFlat;
out vec3 color;

const float pi = 3.14159265359f;

float srgb(float x) {
    return x < 0.0031308 ? 12.92 * x : (1.055 * pow(x, 1.0/2.4) - 0.055);
}

void main() {
    vec3 normal = normalize(shadeFlat ? wsNormalFlat : wsNormal);
    if (gl_FrontFacing) {
        // diffuse shading
        vec3 wsPos = wsPosition.xyz/wsPosition.w;
        vec3 wsDir = pointLightPos-wsPos;
        float dist = length(wsDir);
        float cosTheta = max(0, dot(normal, normalize(wsDir)));

        color = albedo*pointLightPower*(cosTheta/(dist*dist*(4.0f*pi*pi)));

        // convert to sRGB
        color = vec3(srgb(color.r), srgb(color.g), srgb(color.b));
    }
    else {
        color = vec3(0.0);
    }
}
)"s;

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
        vertexBuffer.setBuffer<Point3D>("position", bvhVertices);
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
        vertexBuffer.setBuffer<LineIndices>("lines", bvhIndices);
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
    vertexBuffer.draw("lines", GL_LINES, 0, displayBVHNodes * 12);

    bvhShader.deactivate();
}

std::string readShaderFile(const std::string_view filename)
{
    std::ifstream file{filename.data(), std::ifstream::in};
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (!file)
        throw std::runtime_error("failed to read shader file "s + std::string(filename));
    file.close();
    return buffer.str();
}

MeshShader::MeshShader(const Instance& instance)
    : model{instance.toWorld}, meshShader{readShaderFile("../src/shaders/exercise07.vert"),
                                          readShaderFile("../src/shaders/exercise07.frag"),
                                          readShaderFile("../src/shaders/exercise07.geom")},
      wobbleShader{vertex_shader_wobble, fragment_shader_wobble},
      shadowShader{vertex_shader_flat, fragment_shader_flat}, debugShader{vertex_shader_debug,
                                                                          fragment_shader_debug}
{
    const Mesh& mesh = instance.mesh;

    vertexBuffer.setBuffer<Point3D>("position", mesh.getVertices());
    vertexBuffer.setBuffer<Normal3D>("normal", mesh.getNormals());
    vertexBuffer.setBuffer<TriangleIndices>("triangles", mesh.getFaces());
    if (mesh.getTextureCoordinates().size())
        vertexBuffer.setBuffer<Point2D>("texCoords", mesh.getTextureCoordinates());

    numTriangles = mesh.getFaces().size();
    smoothGroups = mesh.getSmoothGroups();

    if (instance.material.textures.displacement) {
        meshShader = {readShaderFile("../src/shaders/exercise07.vert"),
                      readShaderFile("../src/shaders/exercise07.frag"),
                      readShaderFile("../src/shaders/exercise07.geom"),
                      readShaderFile("../src/shaders/exercise07.tcs"),
                      readShaderFile("../src/shaders/exercise07.tes")};
        tesselate = true;
    }

    meshShader.activate();
    meshShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    meshShader.setVertexAttribute<Normal3D>("normal", vertexBuffer);
    if (mesh.getTextureCoordinates().size()) {
        if (instance.material.isTextured()) {
            meshShader.setVertexAttribute<Point2D>("texCoords", vertexBuffer);
            meshShader.setUniform("textured", true);
            if (tesselate)
                meshShader.setUniform("displacementScale", 0.125f);
        }
    }
    if (instance.material.isRoughConductor()) {
        meshShader.setUniform("ggx", true);

        auto roughConductor = instance.material.roughConductor();
        meshShader.setUniform("albedo", ::Color{0.0f});
        meshShader.setUniform("alpha", roughConductor.microfacetDistribution.alpha);
        meshShader.setUniform("eta", Point3D{roughConductor.conductor.eta});
        meshShader.setUniform("k", Point3D{roughConductor.conductor.k});
    }
    else if (instance.material.isRoughPlastic()) {
        meshShader.setUniform("ggx", true);

        auto roughPlastic = instance.material.roughPlastic();
        meshShader.setUniform("albedo", roughPlastic.diffuse.albedo);
        meshShader.setUniform("alpha", roughPlastic.microfacetDistribution.alpha);
        meshShader.setUniform("eta", Point3D{roughPlastic.dielectric.etaI});
        meshShader.setUniform("k", Point3D{0.0f});
    }
    else {
        meshShader.setUniform("ggx", false);
        meshShader.setUniform("albedo", instance.material.albedo());
    }

    meshShader.deactivate();

    wobbleShader.activate();
    wobbleShader.setUniform("albedo", instance.material.albedo());
    wobbleShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    wobbleShader.setVertexAttribute<Normal3D>("normal", vertexBuffer);
    wobbleShader.deactivate();

    shadowShader.activate();
    shadowShader.setUniform("albedo", instance.material.albedo()); // just for debugging :)
    shadowShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    shadowShader.deactivate();

    debugShader.activate();
    debugShader.setVertexAttribute<Point3D>("position", vertexBuffer);
    debugShader.setVertexAttribute<Normal3D>("normal", vertexBuffer);
    if (mesh.getTextureCoordinates().size())
        debugShader.setVertexAttribute<Point2D>("texCoords", vertexBuffer);
    debugShader.deactivate();

    if (instance.material.textures.albedo)
        albedoTexture = {instance.material.textures.albedo};
    if (instance.material.textures.normal)
        normalTexture = {instance.material.textures.normal};
    if (instance.material.textures.roughness)
        roughnessTexture = {instance.material.textures.roughness};
    if (instance.material.textures.displacement)
        displacementTexture = {instance.material.textures.displacement};
}

void MeshShader::draw(const Light::Point& light, const Point3D cameraPos, const Matrix4D& vp,
                      const Matrix4D& lp, const GLTexture& shadowMap, const Parameters& params)
{
    GLenum mode{GL_TRIANGLES};

    meshShader.activate();

    meshShader.setUniform("mvp", vp * model);
    meshShader.setUniform("mlp", lp * model);
    meshShader.setUniform("model", model);
    meshShader.setUniform("cameraPos", cameraPos);
    meshShader.setUniform("pointLightPos", light.pos);
    meshShader.setUniform("pointLightPower", light.power);
    meshShader.setUniform("useNormalMapping", params.normalMap);
    meshShader.setUniform("useDisplacementMapping", params.displacementMap);
    meshShader.setUniform("useShadowMapping", params.shadowMap);

    glActiveTexture(GL_TEXTURE0);
    shadowMap.bind();
    meshShader.setUniform("shadowMap", 0);

    glActiveTexture(GL_TEXTURE1);
    albedoTexture.bind();
    meshShader.setUniform("albedoTexture", 1);

    glActiveTexture(GL_TEXTURE2);
    normalTexture.bind();
    meshShader.setUniform("normalMap", 2);

    glActiveTexture(GL_TEXTURE3);
    roughnessTexture.bind();
    meshShader.setUniform("roughnessTexture", 3);

    if (tesselate) {
        glActiveTexture(GL_TEXTURE4);
        displacementTexture.bind();
        meshShader.setUniform("displacementMap", 4);

        mode = GL_PATCHES;
    }

    // draw flat parts
    {
        meshShader.setUniform("shadeFlat", true);
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                vertexBuffer.draw("triangles", mode, pos, start);
            pos = end;
        }
        vertexBuffer.draw("triangles", mode, pos, numTriangles);
    }
    // draw smooth parts
    {
        meshShader.setUniform("shadeFlat", false);
        for (auto [start, end] : smoothGroups)
            vertexBuffer.draw("triangles", mode, start, end);
    }

    meshShader.deactivate();
}

void MeshShader::drawWobble(const Light::Point& light, const Matrix4D& vp, float time)
{
    wobbleShader.activate();

    wobbleShader.setUniform("mvp", vp * model);
    wobbleShader.setUniform("model", model);
    wobbleShader.setUniform("pointLightPos", light.pos);
    wobbleShader.setUniform("pointLightPower", light.power);
    wobbleShader.setUniform("time", time);

    // draw flat parts
    {
        wobbleShader.setUniform("shadeFlat", true);
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                vertexBuffer.draw("triangles", GL_TRIANGLES, pos, start);
            pos = end;
        }
        vertexBuffer.draw("triangles", GL_TRIANGLES, pos, numTriangles);
    }
    // draw smooth parts
    {
        wobbleShader.setUniform("shadeFlat", false);
        for (auto [start, end] : smoothGroups)
            vertexBuffer.draw("triangles", GL_TRIANGLES, start, end);
    }

    wobbleShader.deactivate();
}

void MeshShader::drawDebug(const AABB& bounds, const Point3D& cameraPos, RenderMode mode,
                           const Matrix4D& vp)
{
    debugShader.activate();

    debugShader.setUniform("mvp", vp * model);
    debugShader.setUniform("model", model);
    debugShader.setUniform("mode", static_cast<int32_t>(mode) - 1);

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
    else if (mode == RenderMode::UV)
        ;
    else
        throw std::logic_error("invalid render mode for debug shader");

    // draw flat parts
    {
        debugShader.setUniform("shadeFlat", true);
        size_t pos = 0;
        for (auto [start, end] : smoothGroups) {
            if (start > pos)
                vertexBuffer.draw("triangles", GL_TRIANGLES, pos, start);
            pos = end;
        }
        vertexBuffer.draw("triangles", GL_TRIANGLES, pos, numTriangles);
    }
    // draw smooth parts
    {
        debugShader.setUniform("shadeFlat", false);
        for (auto [start, end] : smoothGroups)
            vertexBuffer.draw("triangles", GL_TRIANGLES, start, end);
    }

    debugShader.deactivate();
}

void MeshShader::drawShadow(const Matrix4D& lp)
{
    shadowShader.activate();
    shadowShader.setUniform("mvp", lp * model);
    vertexBuffer.draw("triangles", GL_TRIANGLES, 0, numTriangles);
    shadowShader.deactivate();
}

MyLittleShader::MyLittleShader()
{
    program =
        GLShaderProgram::compileShaderProgram(my_little_vertex_shader, my_little_fragment_shader);
    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
    CHECK_GL();

    const Point2D positions[]{
        {-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}, {-1.0f, -1.0f},
    };
    const Color colors[]{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},
                         {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};

    glBindVertexArray(vertexArray);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    GLuint pos = static_cast<GLuint>(glGetAttribLocation(program, "position"));
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, sizeof(Point2D) / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    GLuint col = static_cast<GLuint>(glGetAttribLocation(program, "color"));
    glEnableVertexAttribArray(col);
    glVertexAttribPointer(col, sizeof(Color) / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
    CHECK_GL();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL();
}

MyLittleShader::~MyLittleShader()
{
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
    CHECK_GL();
}

void MyLittleShader::draw(float time)
{
    // checking if something went wrong somewhere else
    CHECK_GL();

    // glDisable(GL_CULL_FACE);
    // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    glBindVertexArray(vertexArray);
    GLint scale = glGetUniformLocation(program, "scale");
    glUniform1f(scale, 0.5f + std::sin(time) * 0.25f);
    CHECK_GL();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    CHECK_GL();
    glUseProgram(0);
    glBindVertexArray(0);
    CHECK_GL();
}
