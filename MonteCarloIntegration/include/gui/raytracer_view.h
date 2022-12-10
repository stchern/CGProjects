#ifndef RAYTRACER_VIEW_H
#define RAYTRACER_VIEW_H

#include <nanogui/nanogui.h>

#include <gui/camera_controls.h>
#include <gui/gui.h>
#include <render/raytracer.h>

class RayTracerView final : public nanogui::ImageView {
public:
    RayTracerView(Widget* parent, Toolbar* toolbar, RayTracer& rayTracer,
                  nanogui::ref<CameraControls> cameraControls);

    virtual void draw(NVGcontext* context) override;

private:
    RayTracer& rayTracer;
    nanogui::ref<nanogui::Texture> texture;
    nanogui::ref<CameraControls> cameraControls;
    nanogui::ref<nanogui::Widget> rayTracerControls;

    nanogui::ref<nanogui::ProgressBar> progress;

    RayTracerParameters params;
};

#endif // RAYTRACER_VIEW_H
