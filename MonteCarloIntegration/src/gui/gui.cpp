#include <gui/gui.h>

#include <nanogui/opengl.h>

#include <render/scene.h>

#include <gui/mesh_view.h>
#include <gui/raytracer_view.h>

using namespace nanogui;

Toolbar::Toolbar(Widget* parent) : Window{parent, ""}
{
    set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Maximum, 0, 2));
}

void Toolbar::perform_layout(NVGcontext* ctx)
{
    Widget::perform_layout(ctx);

    for (int i = 0; i < child_count(); ++i) {
        const int posX = i * size().x() / child_count();
        const int posY = child_at(i)->position().y();
        if (child_at(i)->position().x() < posX) {
            child_at(i)->set_position({posX, posY});
            child_at(i)->perform_layout(ctx);
        }
    }
}

GDVGUI::GDVGUI(Screen* screen) : Widget(screen)
{
    set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 2));
    set_fixed_size(screen->framebuffer_size());

    menubar = new Toolbar(screen);
    statusbar = new Toolbar(screen);

    cameraControls = new CameraControls{statusbar};
}

void GDVGUI::perform_layout(NVGcontext* ctx)
{
    const Vector2i mainWidgetSize = {(m_size.x() - 1) / static_cast<int>(mainWidgets.size()),
                                     m_size.y()};
    for (Widget* widget : mainWidgets)
        widget->set_fixed_size(mainWidgetSize);

    // m_leftCanvas->set_position({0, 0});
    // m_rightCanvas->set_position(size - canvasSize);
    menubar->set_position({0, 0});
    menubar->set_fixed_width(m_size.x());
    statusbar->set_position({0, m_size.y() - statusbar->size().y()});
    statusbar->set_fixed_width(m_size.x());

    Widget::perform_layout(ctx);
}

bool GDVGUI::keyboard_event(int key, int scancode, int action, int modifiers)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        return cameraControls->handleKeypress(key);

    return false;

    (void)scancode, (void)modifiers; // unused
}

bool GDVGUI::scroll_event(const Vector2i& p, const Vector2f& rel)
{
    if (Widget::scroll_event(p, rel))
        return true;

    cameraControls->zoom(-rel.y());

    return true;
}

void GDVGUI::addMeshView(const Scene& scene)
{
    mainWidgets.push_back(new MeshView(this, menubar, scene, cameraControls));
}

void GDVGUI::addRayTracerView(RayTracer& rayTracer)
{
    mainWidgets.push_back(new RayTracerView(this, menubar, rayTracer, cameraControls));
}

void GDVGUI::setCameraParameters(const CameraParameters& params)
{
    cameraControls->setCameraParameters(params);
}
