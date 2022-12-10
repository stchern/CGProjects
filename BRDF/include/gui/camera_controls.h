#ifndef CAMERA_CONTROLS_H
#define CAMERA_CONTROLS_H

#include <nanogui/nanogui.h>

#include <render/camera.h>

class CameraControls final : public nanogui::Widget {
public:
    CameraControls(nanogui::FormHelper& gui, nanogui::Widget* parent, nanogui::Vector2i windowPos);

    CameraParameters getCameraParameters(Resolution res) const;

    void refresh();

    void zoom(float diff)
    {
        params.perspective.fov += diff;
        refresh();
    }

    bool handleKeypress(int key);

private:
    CameraParameters params;
    mutable float cameraTime{0.0f};
    mutable float lastTime{0.0f};
    bool autoAdjustFOV{true};
    bool rotateCamera{false};

    nanogui::ref<nanogui::Window> advancedControlWindow;
    nanogui::ref<nanogui::Widget> pos, target, up, perspective;
};

#endif // CAMERA_CONTROLS_H
