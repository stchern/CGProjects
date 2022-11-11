#ifndef RAYTRACER_VIEW_H
#define RAYTRACER_VIEW_H

#include <nanogui/imageview.h>
#include <nanogui/texture.h>

#include "camera_parameters.h"
#include "film.h"
#include "raytracer.h"

#include <memory>

class RayTracerView : public nanogui::ImageView {
public:
    RayTracerView(Widget* parent) : ImageView(parent)
    {
        set_pixel_callback([&](const nanogui::Vector2i& index, char** out, size_t size) {
            const auto& pixels = rayTracer->getFilm().getPixels();
            for (uint8_t ch = 0; ch < 4; ++ch) {
                const size_t i =
                    static_cast<size_t>(index.x())
                    + static_cast<size_t>(index.y()) * static_cast<size_t>(texture->size().x());
                const float value = pixels.at(i).v[ch];
                snprintf(out[ch], size, "%f", value);
            }
        });
    }

    virtual void draw(NVGcontext* context) override;

    void setScene(std::unique_ptr<Scene>&& scene)
    {
        rayTracer = std::make_unique<RayTracer>(std::move(scene));
    }
    void setCameraControls(const nanogui::ref<CameraControls> controls)
    {
        cameraControls = controls;
    }
    void restart();

private:
    std::unique_ptr<RayTracer> rayTracer;
    nanogui::ref<nanogui::Texture> texture;
    nanogui::ref<CameraControls> cameraControls;
};

class RayTracerControls final : public nanogui::Object {
public:
    RayTracerControls(nanogui::Widget* statusbar, const nanogui::ref<RayTracerView>& rayTracerView)
        : rayTracerView{rayTracerView}
    {
        using nanogui::Label, nanogui::Button;
        using nanogui::Widget, nanogui::BoxLayout, nanogui::Orientation, nanogui::Alignment;

        auto rayTraceControls = new Widget(statusbar);
        rayTraceControls->set_layout(
            new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 2));
        new Widget(rayTraceControls); // spacer
        new Label(rayTraceControls, "Ray Tracer");
        auto rerender = new Button(rayTraceControls, "(Re-)render", FA_PLAY);
        rerender->set_callback([&]() -> void { this->rayTracerView->restart(); });
        auto resetView = new Button(rayTraceControls, "Reset View", FA_VECTOR_SQUARE);
        resetView->set_callback([&]() -> void { this->rayTracerView->reset(); });
    }

private:
    nanogui::ref<nanogui::Window> controlWindow;
    nanogui::ref<RayTracerView> rayTracerView;
};

#endif // !RAYTRACER_VIEW_H
