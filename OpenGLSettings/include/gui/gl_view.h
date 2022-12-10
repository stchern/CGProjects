#ifndef GL_VIEW_H
#define GL_VIEW_H

#include <geometry/mesh.h>
#include <gui/gui.h>
#include <render/gl_shader.h>

struct MeshDisplayParameters {
    bool wireframe{false};
    bool showAxes{false};
    bool rotatePointLight{false};
    int16_t bvhLevel{0};

    MeshShader::RenderMode mode{MeshShader::RenderMode::Shaded};

    Light::Point pointLight{};
};

/// A class to display 3D meshes
class GLView final : public nanogui::Canvas {
public:
    GLView(nanogui::Widget* parent, Toolbar* toolbar, const Scene& scene,
           nanogui::ref<CameraControls> cameraControls);

    void setScene(const Scene& scene);

    virtual void draw_contents() override;

private:
    BackgroundShader backgroundShader;
    MyLittleShader toyShader;

    Mesh axesMesh;
    MeshShader axesShader;
    AABB sceneAABB;

    nanogui::ref<CameraControls> cameraControls;
    nanogui::ref<nanogui::Widget> meshDisplayControls;

    std::vector<MeshShader> meshes;
    std::vector<BVHShader> bvhs;
    MeshDisplayParameters params;

    Resolution shadowResolution{2048, 2048};
    nanogui::ref<nanogui::Texture> shadowColorTexture;
    nanogui::ref<nanogui::Texture> shadowTexture;
    nanogui::ref<nanogui::RenderPass> shadowPass;
};

#endif // GL_VIEW_H
