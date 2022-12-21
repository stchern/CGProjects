/*
    Framework for GDV exercises, based on NanoGUI.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <iostream>
#include <nanogui/nanogui.h>
// includes windows.h on Windows
#include <nanogui/opengl.h>

#include <gui/gui.h>
#include <gui/image_processing_view.h>

class GDVApplication final : public nanogui::Screen {
public:
    GDVApplication(const nanogui::Vector2i& size)
        : nanogui::Screen{size,
                          "GDV 2022/23",
                          /* resizable */ true,
                          /* fullscreen */ false,
                          /* depth_buffer */ true,
                          /* stencil_buffer */ true,
                          /* float_buffer */ false,
                          /* gl_major */ 4,
                          /* gl_minor */ 1}
    {
        inc_ref();

        gui = new GDVGUI(this);

        // TODO: enable this for the DFT task
        if constexpr (/* Discrete Fourier Transform */ (true)) {
            auto view1 = gui->addDFTView();
            auto view2 = gui->addDFTView();
            view1->setNext(view2);
            view2->setNext(view1);
        }
        // TODO: enable this for the supersampling task
        else if constexpr (/* Supersampling */ (true)) {
            gui->addSamplingView();
        }

        perform_layout();
        resize_event(framebuffer_size());
    }

    bool resize_event(const nanogui::Vector2i& size) override
    {
        gui->set_fixed_size(framebuffer_size());
        perform_layout();
        Screen::resize_event(size);

        return true;
    }

private:
    nanogui::ref<GDVGUI> gui;
};

int main(int /* argc */, char** /* argv */)
{
    try {
        nanogui::init();

        /* scoped variables */ {
            nanogui::ref<GDVApplication> app = new GDVApplication({1024, 768});
            app->dec_ref();
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(1 / 60.f * 1000);
        }

        nanogui::shutdown();
    }
    catch (const std::exception& e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
        MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
        std::cerr << error_msg << std::endl;
#endif
        return -1;
    }
    catch (...) {
        std::cerr << "Caught an unknown error!" << std::endl;
    }

    return 0;
}
