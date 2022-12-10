#include <gui/camera_controls.h>

#include <nanogui/opengl.h>

using namespace nanogui;

CameraControls::CameraControls(FormHelper& gui, Widget* parent, Vector2i windowPos) : Widget(parent)
{
    set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 2));

    (new Widget(this))->set_fixed_width(8); // spacer
    auto icon = new Label(this, utf8(FA_CAMERA));
    icon->set_font("icons");
    new Label(this, "Camera");
    (new Widget(this))->set_fixed_width(16); // spacer

    auto floatSelect = [](Widget* parent, float& value) {
        auto x = new FloatBox<float>(parent, value);
        x->set_callback([&](float x) -> void { value = x; });
        x->set_font_size(16);
        x->number_format("%.3g");
        x->set_fixed_width(60);
        x->set_alignment(TextBox::Alignment::Right);
        x->set_editable(true);
        x->set_spinnable(true);
        return x;
    };

    auto xyzSelect = [&](Widget* parent, const std::string& label, Point3D& value) {
        auto xyz = new Widget(parent);
        xyz->set_layout(new BoxLayout(Orientation::Horizontal));
        new Label(xyz, label);
        floatSelect(xyz, value.x);
        floatSelect(xyz, value.y);
        floatSelect(xyz, value.z);
        return xyz;
    };

    new Widget(this); // spacer
    pos = xyzSelect(this, "Pos", params.pos);
    target = xyzSelect(this, "Target", params.target);
    up = xyzSelect(this, "Up", params.up);

    perspective = new Widget(this);
    perspective->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(perspective, "FOV");
    floatSelect(perspective, params.perspective.fov);

    auto more = new Button(this, "", FA_ELLIPSIS_H);
    more->set_font_size(16);
    more->set_callback([&]() -> void {
        advancedControlWindow->set_visible(true);
        advancedControlWindow->request_focus();
    });

    advancedControlWindow = gui.add_window(windowPos, "Camera Parameters");
    gui.add_group("Orthographic");
    gui.add_variable("left", params.orthographic.left)->set_spinnable(true);
    gui.add_variable("right", params.orthographic.right)->set_spinnable(true);
    gui.add_variable("top", params.orthographic.top)->set_spinnable(true);
    gui.add_variable("bottom", params.orthographic.bottom)->set_spinnable(true);
    gui.add_group("Options");
    gui.add_variable("type", params.type)
        ->set_items({"Perspective", "Orthographic"}, {"Persp", "Ortho"});
    gui.add_variable("auto aspect", autoAdjustFOV)
        ->set_tooltip("auto-adjust field of view / top/bottom for narrow aspect ratios");
    gui.add_variable("rotate", rotateCamera)->set_tooltip("rotate camera on the zx-plane");
    gui.add_button("reset rotation", [&]() -> void { cameraTime = 0.0f; });
    advancedControlWindow->set_visible(false);
    auto closeButton = new Button{advancedControlWindow->button_panel(), "", FA_TIMES};
    closeButton->set_callback([&]() -> void { advancedControlWindow->set_visible(false); });
}

CameraParameters CameraControls::getCameraParameters(Resolution res) const
{
    CameraParameters result{params};
    const float prev = lastTime;
    lastTime = static_cast<float>(glfwGetTime());
    if (rotateCamera)
        cameraTime += lastTime - prev;
    const float sin = std::sin(cameraTime);
    const float cos = std::cos(cameraTime);
    result.pos.x = sin * params.pos.z + cos * params.pos.x;
    result.pos.z = cos * params.pos.z - sin * params.pos.x;

    result.resolution = res;
    // scale fov when the aspect ratio is weird
    if (autoAdjustFOV) {
        result.perspective.fov /= std::max(0.2f, std::min(1.0f, result.aspect()));

        result.orthographic.top /= result.aspect();
        result.orthographic.bottom /= result.aspect();
    }
    return result;
}

void CameraControls::refresh()
{
    dynamic_cast<FloatBox<float>*>(pos->child_at(1))->set_value(params.pos.x);
    dynamic_cast<FloatBox<float>*>(pos->child_at(2))->set_value(params.pos.y);
    dynamic_cast<FloatBox<float>*>(pos->child_at(3))->set_value(params.pos.z);

    dynamic_cast<FloatBox<float>*>(target->child_at(1))->set_value(params.target.x);
    dynamic_cast<FloatBox<float>*>(target->child_at(2))->set_value(params.target.y);
    dynamic_cast<FloatBox<float>*>(target->child_at(3))->set_value(params.target.z);

    dynamic_cast<FloatBox<float>*>(up->child_at(1))->set_value(params.up.x);
    dynamic_cast<FloatBox<float>*>(up->child_at(2))->set_value(params.up.y);
    dynamic_cast<FloatBox<float>*>(up->child_at(3))->set_value(params.up.z);

    dynamic_cast<FloatBox<float>*>(perspective->child_at(1))->set_value(params.perspective.fov);
}

bool CameraControls::handleKeypress(int key)
{
    const float moveDelta = 0.1f;
    const float rotateDelta = 0.05f;

    auto move = [&](Vector3D offset) {
        params.pos += offset;
        params.target += offset;
        refresh();
    };

    // FIXME: angular rotation
    auto rotate = [&](Vector3D offset) {
        params.target += offset * distance(params.pos, params.target);
        refresh();
    };

    switch (key) {
    case GLFW_KEY_UP:
    case GLFW_KEY_W:
        move(params.computeFrame().dir * moveDelta);
        break;
    case GLFW_KEY_DOWN:
    case GLFW_KEY_S:
        move(-params.computeFrame().dir * moveDelta);
        break;
    case GLFW_KEY_Q:
        move(-params.computeFrame().right * moveDelta);
        break;
    case GLFW_KEY_E:
        move(params.computeFrame().right * moveDelta);
        break;
    case GLFW_KEY_SPACE:
        move(params.up * moveDelta);
        break;
    case GLFW_KEY_X:
        move(-params.up * moveDelta);
        break;
    case GLFW_KEY_LEFT:
    case GLFW_KEY_A:
        rotate(-params.computeFrame().right * rotateDelta);
        break;
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_D:
        rotate(params.computeFrame().right * rotateDelta);
        break;
    case GLFW_KEY_INSERT:
        rotate(params.up * rotateDelta);
        break;
    case GLFW_KEY_DELETE:
        rotate(-params.up * rotateDelta);
        break;
    case GLFW_KEY_KP_DECIMAL: {
        // TODO: center and scale
        break;
    }
    default:
        return false;
    }
    return true;
}
