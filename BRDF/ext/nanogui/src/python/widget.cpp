#ifdef NANOGUI_PYTHON

#include "python.h"
#include <nanobind/make_iterator.h>

DECLARE_WIDGET(Widget);
DECLARE_SCREEN(Screen);
DECLARE_WIDGET(Window);

/// Cyclic GC support: find all callback getter functions and traverse them recursively
int widget_tp_traverse_base(PyObject *self, visitproc visit, void *arg, PyTypeObject *tp) {
    PyObject *dict = tp->tp_dict;
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    const char *suffix = "callback";
    size_t suffix_len = strlen(suffix);

    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (!PyUnicode_Check(key))
            continue;

        const char *name = PyUnicode_AsUTF8AndSize(key, nullptr);
        size_t name_len = strlen(name);

        if (suffix_len > name_len)
            continue;

        if (strncmp(name, "set_", 4) == 0 ||
            strcmp(name + name_len - suffix_len, suffix) != 0)
            continue;


#if PY_VERSION_HEX < 0x03090000
        PyObject *func = PyObject_GetAttr(self, key),
                 *result = nullptr;
        if (func) {
            result = _PyObject_Vectorcall(func, nullptr, 0, nullptr);
            Py_DECREF(func);
        }
#else
        PyObject *args[] = { self };
        PyObject *result = PyObject_VectorcallMethod(
            key, args, 1 | PY_VECTORCALL_ARGUMENTS_OFFSET, nullptr);
#endif

        if (!result) {
            PyErr_Clear();
            continue;
        }

        Py_VISIT(result);
        Py_DECREF(result);
    }

    if (strcmp(tp->tp_name, "nanogui.Widget") != 0) {
        PyTypeObject *base = (PyTypeObject *) PyType_GetSlot(tp, Py_tp_base);
        if (base) {
            int rv = widget_tp_traverse_base(self, visit, arg, base);
            if (rv)
                return rv;
        }
    }

    return 0;
}

// Main widget traversal method that may be invoked by Python's cyclic GC
int widget_tp_traverse(PyObject *self, visitproc visit, void *arg) {

    Widget *w = nb::inst_ptr<Widget>(self);

    // Visit nested children
    for (Widget *wc : w->children()) {
        PyObject *o = wc->self_py();
        Py_VISIT(o);
    }

    // Visit callback functions (which can also produce reference cycles)
    return widget_tp_traverse_base(self, visit, arg, Py_TYPE(self));
}

int widget_tp_clear(PyObject *self) {
    Widget *w = nb::inst_ptr<Widget>(self);

    int count = w->child_count();
    for (int i = 0; i < count; ++i)
        w->remove_child_at(count - 1 - i);

    return 0;
}

void register_widget(nb::module_ &m) {
    object_init_py(
        [](PyObject *o) noexcept {
            nb::gil_scoped_acquire guard;
            Py_INCREF(o);
        },
        [](PyObject *o) noexcept {
            nb::gil_scoped_acquire guard;
            Py_DECREF(o);
        });

    nb::class_<Object>(
        m, "Object",
        nb::intrusive_ptr<Object>(
            [](Object *o, PyObject *po) noexcept { o->set_self_py(po); }));

    PyType_Slot widget_type_slots[] = {
        { Py_tp_traverse, (void *) widget_tp_traverse },
        { Py_tp_clear, (void *) widget_tp_clear },
        { 0, nullptr }
    };

    nb::class_<Widget, Object, PyWidget>(
        m, "Widget", D(Widget), nb::type_slots(widget_type_slots))
        .def(nb::init<Widget *>(), D(Widget, Widget))
        .def("parent", (Widget *(Widget::*)(void)) &Widget::parent, D(Widget, parent))
        .def("set_parent", &Widget::set_parent, D(Widget, set_parent))
        .def("layout", (Layout *(Widget::*)(void)) &Widget::layout, D(Widget, layout))
        .def("set_layout", &Widget::set_layout, D(Widget, set_layout))
        .def("theme", (Theme *(Widget::*)(void)) &Widget::theme, D(Widget, theme))
        .def("set_theme", &Widget::set_theme, D(Widget, set_theme))
        .def("position", &Widget::position, D(Widget, position))
        .def("set_position", &Widget::set_position, D(Widget, set_position))
        .def("absolute_position", &Widget::absolute_position, D(Widget, absolute_position))
        .def("size", &Widget::size, D(Widget, size))
        .def("set_size", &Widget::set_size, D(Widget, set_size))
        .def("width", &Widget::width, D(Widget, width))
        .def("set_width", &Widget::set_width, D(Widget, set_width))
        .def("height", &Widget::height, D(Widget, height))
        .def("set_height", &Widget::set_height, D(Widget, set_height))
        .def("fixed_size", &Widget::fixed_size, D(Widget, fixed_size))
        .def("set_fixed_size", &Widget::set_fixed_size, D(Widget, set_fixed_size))
        .def("fixed_width", &Widget::fixed_width, D(Widget, fixed_width))
        .def("set_fixed_width", &Widget::set_fixed_width, D(Widget, set_fixed_width))
        .def("fixed_height", &Widget::fixed_height, D(Widget, fixed_height))
        .def("set_fixed_height", &Widget::set_fixed_height, D(Widget, set_fixed_height))
        .def("visible", &Widget::visible, D(Widget, visible))
        .def("set_visible", &Widget::set_visible, D(Widget, set_visible))
        .def("visible_recursive", &Widget::visible_recursive, D(Widget, visible_recursive))
        .def("children", (const std::vector<Widget *>&(Widget::*)(void)) &Widget::children,
             D(Widget, children), nb::rv_policy::reference)
        .def("add_child", (void (Widget::*) (int, Widget *)) &Widget::add_child, D(Widget, add_child))
        .def("add_child", (void (Widget::*) (Widget *)) &Widget::add_child, D(Widget, add_child, 2))
        .def("child_count", &Widget::child_count, D(Widget, child_count))
        .def("__len__", &Widget::child_count, D(Widget, child_count))
        .def("__iter__", [](const Widget &w) {
                return nb::make_iterator(nb::type<Widget>(), "iterator", w.children().begin(), w.children().end());
            }, nb::keep_alive<0, 1>())
        .def("child_index", &Widget::child_index, D(Widget, child_index))
        .def("__getitem__", (Widget* (Widget::*)(int)) &Widget::child_at, D(Widget, child_at))
        .def("remove_child_at", &Widget::remove_child_at, D(Widget, remove_child_at))
        .def("remove_child", &Widget::remove_child, D(Widget, remove_child))
        .def("__delitem__", &Widget::remove_child_at, D(Widget, remove_child_at))
        .def("enabled", &Widget::enabled, D(Widget, enabled))
        .def("set_enabled", &Widget::set_enabled, D(Widget, set_enabled))
        .def("focused", &Widget::focused, D(Widget, focused))
        .def("set_focused", &Widget::set_focused, D(Widget, set_focused))
        .def("request_focus", &Widget::request_focus, D(Widget, request_focus))
        .def("tooltip", &Widget::tooltip, D(Widget, tooltip))
        .def("set_tooltip", &Widget::set_tooltip, D(Widget, set_tooltip))
        .def("font_size", &Widget::font_size, D(Widget, font_size))
        .def("set_font_size", &Widget::set_font_size, D(Widget, set_font_size))
        .def("has_font_size", &Widget::has_font_size, D(Widget, has_font_size))
        .def("cursor", &Widget::cursor, D(Widget, cursor))
        .def("set_cursor", &Widget::set_cursor, D(Widget, set_cursor))
        .def("find_widget", (Widget *(Widget::*)(const Vector2i &)) &Widget::find_widget, D(Widget, find_widget))
        .def("contains", &Widget::contains, D(Widget, contains))
        .def("mouse_button_event", &Widget::mouse_button_event, "p"_a, "button"_a,
             "down"_a, "modifiers"_a, D(Widget, mouse_button_event))
        .def("mouse_motion_event", &Widget::mouse_motion_event, "p"_a, "rel"_a,
             "button"_a, "modifiers"_a, D(Widget, mouse_motion_event))
        .def("mouse_drag_event", &Widget::mouse_drag_event, "p"_a, "rel"_a,
             "button"_a, "modifiers"_a, D(Widget, mouse_drag_event))
        .def("mouse_enter_event", &Widget::mouse_enter_event, "p"_a, "enter"_a,
             D(Widget, mouse_enter_event))
        .def("scroll_event", &Widget::scroll_event, "p"_a, "rel"_a,
             D(Widget, scroll_event))
        .def("focus_event", &Widget::focus_event, "focused"_a, D(Widget, focus_event))
        .def("keyboard_event", &Widget::keyboard_event, "key"_a, "scancode"_a,
             "action"_a, "modifiers"_a, D(Widget, keyboard_event))
        .def("keyboard_character_event", &Widget::keyboard_character_event,
             D(Widget, keyboard_character_event))
        .def("preferred_size", &Widget::preferred_size, D(Widget, preferred_size))
        .def("perform_layout", &Widget::perform_layout, D(Widget, perform_layout))
        .def("screen", nb::overload_cast<>(&Widget::screen, nb::const_), D(Widget, screen))
        .def("window", nb::overload_cast<>(&Widget::window, nb::const_), D(Widget, window))
        .def("draw", &Widget::draw, D(Widget, draw));

    nb::class_<Window, Widget, PyWindow>(m, "Window", D(Window))
        .def(nb::init<Widget *, const std::string>(), "parent"_a,
             "title"_a = std::string("Untitled"), D(Window, Window))
        .def("title", &Window::title, D(Window, title))
        .def("set_title", &Window::set_title, D(Window, set_title))
        .def("modal", &Window::modal, D(Window, modal))
        .def("set_modal", &Window::set_modal, D(Window, set_modal))
        .def("dispose", &Window::dispose, D(Window, dispose))
        .def("button_panel", &Window::button_panel, D(Window, button_panel))
        .def("center", &Window::center, D(Window, center));

    nb::class_<Screen, Widget, PyScreen>(m, "Screen", D(Screen))
        .def(nb::init<const Vector2i &, const std::string &, bool, bool, bool,
                      bool, bool, unsigned int, unsigned int>(),
            "size"_a, "caption"_a = "Unnamed", "resizable"_a = true, "fullscreen"_a = false,
            "depth_buffer"_a = true, "stencil_buffer"_a = true,
            "float_buffer"_a = false, "gl_major"_a = 3, "gl_minor"_a = 2, D(Screen, Screen))
        .def("caption", &Screen::caption, D(Screen, caption))
        .def("set_caption", &Screen::set_caption, D(Screen, set_caption))
        .def("background", &Screen::background, D(Screen, background))
        .def("set_background", &Screen::set_background, D(Screen, set_background))
        .def("set_visible", &Screen::set_visible, D(Screen, set_visible))
        .def("set_size", &Screen::set_size, D(Screen, set_size))
        .def("framebuffer_size", &Screen::framebuffer_size, D(Screen, framebuffer_size))
        .def("perform_layout", (void(Screen::*)(void)) &Screen::perform_layout, D(Screen, perform_layout))
        .def("redraw", &Screen::redraw, D(Screen, redraw))
        .def("clear", &Screen::clear, D(Screen, clear))
        .def("draw_all", &Screen::draw_all, D(Screen, draw_all))
        .def("draw_contents", &Screen::draw_contents, D(Screen, draw_contents))
        .def("resize_event", &Screen::resize_event, "size"_a, D(Screen, resize_event))
        .def("resize_callback", &Screen::resize_callback)
        .def("set_resize_callback", &Screen::set_resize_callback)
        .def("drop_event", &Screen::drop_event, D(Screen, drop_event))
        .def("mouse_pos", &Screen::mouse_pos, D(Screen, mouse_pos))
        .def("pixel_ratio", &Screen::pixel_ratio, D(Screen, pixel_ratio))
        .def("has_depth_buffer", &Screen::has_depth_buffer, D(Screen, has_depth_buffer))
        .def("has_stencil_buffer", &Screen::has_stencil_buffer, D(Screen, has_stencil_buffer))
        .def("has_float_buffer", &Screen::has_float_buffer, D(Screen, has_float_buffer))
        .def("glfw_window", &Screen::glfw_window, D(Screen, glfw_window),
                nb::rv_policy::reference)
        .def("nvg_context", &Screen::nvg_context, D(Screen, nvg_context),
                nb::rv_policy::reference)
        .def("pixel_format", &Screen::pixel_format, D(Screen, pixel_format))
        .def("component_format", &Screen::component_format, D(Screen, component_format))
        .def("nvg_flush", &Screen::nvg_flush, D(Screen, nvg_flush))
#if defined(NANOGUI_USE_METAL)
        .def("metal_layer", &Screen::metal_layer)
        .def("metal_texture", &Screen::metal_texture)
        .def("depth_stencil_texture", &Screen::depth_stencil_texture)
#endif
        ;
}
#endif
