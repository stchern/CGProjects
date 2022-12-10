#ifndef MESH_SHADER_H
#define MESH_SHADER_H

#include <nanogui/object.h> // ref
#include <nanogui/shader.h> // Shader
#include <nanogui/vector.h> // Matrix4f

struct MeshDisplayParameters {
    bool wireframe{false};
    bool normals{false};
    bool showAxes{false};
    int16_t bvhLevel{0};
};

struct Instance;

class MeshShader final {
public:
    MeshShader(const Instance& instance, nanogui::RenderPass* render_pass);

    void draw(const MeshDisplayParameters& displayParams, const nanogui::Matrix4f& vp);

    /// toWorld transformation matrix
    nanogui::Matrix4f model;

private:
    std::vector<std::pair<size_t, size_t>> smoothGroups;
    nanogui::ref<nanogui::Shader> meshShader;
    nanogui::ref<nanogui::Shader> bvhShader;
    size_t numTriangles{0};
    size_t numBVHNodes{0};
};

#endif // MESH_SHADER_H
