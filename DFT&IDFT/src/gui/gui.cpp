#include <gui/gui.h>

#include <nanogui/opengl.h>

#include <gui/image_processing_view.h>

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
    if (statusbar) {
        statusbar->set_position({0, m_size.y() - statusbar->size().y()});
        statusbar->set_fixed_width(m_size.x());
    }

    Widget::perform_layout(ctx);
}

bool GDVGUI::keyboard_event(int key, int scancode, int action, int modifiers)
{
    return false;

    (void)scancode, (void)modifiers; // unused
}

bool GDVGUI::scroll_event(const Vector2i& p, const Vector2f& rel)
{
    if (Widget::scroll_event(p, rel))
        return true;

    return true;
}

DFTView* GDVGUI::addDFTView()
{
    DFTView* view = new DFTView(this, menubar);
    mainWidgets.push_back(view);
    return view;
}

SamplingView* GDVGUI::addSamplingView()
{
    SamplingView* view = new SamplingView(this, menubar);
    mainWidgets.push_back(view);
    return view;
}
