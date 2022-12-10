#ifndef GUI_H
#define GUI_H

#include <nanogui/nanogui.h>

#include <geometry/mesh.h>
#include <render/raytracer.h>

class CameraControls;
class MeshView;

class Toolbar : public nanogui::Window {
public:
    Toolbar(nanogui::Widget* parent);
    void perform_layout(NVGcontext* ctx) override;
};

class GDVGUI final : public nanogui::Widget {
public:
    GDVGUI(nanogui::Screen* screen);

    void perform_layout(NVGcontext* ctx) override;

    bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;

    void addMeshView(const Scene& scene);
    void addRayTracerView(RayTracer& rayTracer);
    void setCameraParameters(const CameraParameters& params);

private:
    std::vector<nanogui::ref<nanogui::Widget>> mainWidgets;
    nanogui::ref<Toolbar> menubar;
    nanogui::ref<Toolbar> statusbar;
    nanogui::ref<CameraControls> cameraControls;
};

#endif // GUI_H
