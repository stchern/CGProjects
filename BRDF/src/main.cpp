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

#include <render/material.h>
#include <render/raytracer.h>
#include <render/scene.h>

#include <gui/camera_controls.h>
#include <gui/gui.h>
#include <gui/mesh_view.h>
#include <gui/raytracer_view.h>

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

        // setup the scene
        {

            Scene scene;

            // Materials
            const Material boxRed{Material::Diffuse{Color{0.8f, 0.4f, 0.3f, 1.0f}}};
            const Material boxGreen{Material::Diffuse{Color{0.4f, 0.8f, 0.3f, 1.0f}}};
            const Material boxWhite{Material::Diffuse{Color{1.0f, 1.0f, 1.0f, 1.0f}}};

            // Instances for scene
            scene.addInstance("../meshes/CubeTobBackBottom.obj", boxWhite);
            scene.addInstance("../meshes/CubeLeft.obj", boxRed);
            scene.addInstance("../meshes/CubeRight.obj", boxGreen);

            // selection of Materials
            const Material diffuseGray{Material::Diffuse{Color{0.5f, 0.5f, 0.5f, 1.0f}}};
            const Material diffuseBlue{Material::Diffuse{Color{0.2f, 0.5f, 0.9f, 0.1f}}};
            const Material water{Material::Dielectric{1.33f, 1.0f}};
            const Material glass{Material::Dielectric{1.5f, 1.0f}};
            const Material gold{Material::Conductor{{0.143085f, 0.374852f, 1.44208f, 1.f},
                                                    {3.98205f, 2.38506f, 1.60276f}}};
            const Material silver{Material::Conductor{{0.15522f, 0.116692f, 0.138342f},
                                                      {4.827f, 3.12139f, 2.14636f}}};
            const Material copper{Material::Conductor{{0.20038f, 0.923777f, 1.10191f, 1.0f},
                                                      {3.91185f, 2.45217f, 2.14159f, 1.0f}}};

            // selection of models...
            // TODO change materials if you wish
            scene.addInstance("../meshes/bunny.obj", diffuseGray,
                              {Matrix3D::scale(5.f), {0.0f, -0.17f, 0.0f}});
            // scene.addInstance("../meshes/gdv.obj", glass, {{}, {0.0f, 0.5f, 0.0f}});
            // scene.addInstance("../meshes/loki.obj", gold);
            // scene.addInstance("../meshes/Su_Laegildah.obj", diffuseGray);

            scene.addLight({500.0f, {0.0f, 3.84f, 2.0f}});
            gui->addMeshView(scene);
            rayTracer.setScene(std::move(scene));
            gui->addRayTracerView(rayTracer);
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
    RayTracer rayTracer;
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
