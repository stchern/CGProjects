#ifndef MESH_VIEW_H
#define MESH_VIEW_H

// include nanogui first to enable definition of conversion operators for interoperability
#include <nanogui/nanogui.h>

#include <geometry/mesh.h>
#include <gui/gui.h>
#include <render/mesh_shader.h>

/// A class to display 3D meshes
class MeshView final : public nanogui::Canvas {
public:
    MeshView(nanogui::Widget* parent, Toolbar* toolbar, const Scene& scene,
             nanogui::ref<CameraControls> cameraControls);

    void setScene(const Scene& scene);

    virtual void draw_contents() override;

private:
    nanogui::ref<nanogui::Shader> backgroundShader;
    Mesh axesMesh;
    MeshShader axesShader;

    nanogui::ref<CameraControls> cameraControls;
    nanogui::ref<nanogui::Widget> meshDisplayControls;

    std::vector<MeshShader> meshes;
    MeshDisplayParameters params;
};

#endif // MESH_VIEW_H
