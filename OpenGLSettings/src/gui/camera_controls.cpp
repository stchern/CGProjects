#include <gui/camera_controls.h>

#include <nanogui/opengl.h>

using namespace nanogui;

CameraControls::CameraControls(Widget* parent) : Widget(parent)
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

    auto more = new PopupButton(this, "", FA_ELLIPSIS_H);
    more->set_font_size(16);
    more->set_side(Popup::Up);

    Popup* popup = more->popup();
    popup->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 8, 4));

    Widget* wrapper = new Widget(popup);
    wrapper->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Maximum, 0, 4));

    Button* perspectiveRadio = new Button(wrapper, "Perspective");
    perspectiveRadio->set_callback(
        [&]() -> void { params.type = CameraParameters::CameraType::Perspective; });
    perspectiveRadio->set_flags(Button::RadioButton);
    perspectiveRadio->set_pushed(params.type == CameraParameters::CameraType::Perspective);

    this->perspective = new Widget(wrapper);
    this->perspective->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(this->perspective, "FOV");
    floatSelect(this->perspective, params.perspective.fov);

    wrapper = new Widget(popup);
    wrapper->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Maximum, 0, 4));

    Button* orthographicRadio = new Button(wrapper, "Orthographic");
    orthographicRadio->set_callback(
        [&]() -> void { params.type = CameraParameters::CameraType::Orthographic; });
    orthographicRadio->set_flags(Button::RadioButton);
    orthographicRadio->set_pushed(params.type == CameraParameters::CameraType::Orthographic);

    perspectiveRadio->set_button_group({perspectiveRadio, orthographicRadio});
    orthographicRadio->set_button_group({perspectiveRadio, orthographicRadio});

    Widget* line = new Widget(wrapper);
    line->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(line, "left");
    floatSelect(line, params.orthographic.left);

    line = new Widget(wrapper);
    line->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(line, "right");
    floatSelect(line, params.orthographic.right);

    line = new Widget(wrapper);
    line->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(line, "top");
    floatSelect(line, params.orthographic.top);

    line = new Widget(wrapper);
    line->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(line, "bottom");
    floatSelect(line, params.orthographic.bottom);

    wrapper = new Widget(popup);
    wrapper->set_layout(new GroupLayout(0, 4, 2, 2));
    new Label(wrapper, "Options", "sans-bold", 20);

    Widget* wrapper2 = new Widget(wrapper);
    wrapper2->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Maximum, 0, 4));
    line = new Widget(wrapper2);
    line->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(line, "near");
    floatSelect(line, params.tNear);
    line = new Widget(wrapper2);
    line->set_layout(new BoxLayout(Orientation::Horizontal));
    new Label(line, "far");
    floatSelect(line, params.tFar);

    auto autoAspect =
        new CheckBox(wrapper, "auto aspect", [&](bool b) -> void { autoAdjustFOV = b; });
    autoAspect->set_tooltip("auto-adjust field of view / top/bottom for narrow aspect ratios");
    autoAspect->set_font_size(16);
    autoAspect->set_checked(autoAdjustFOV);
    auto rotate = new CheckBox(wrapper, "rotate", [&](bool b) -> void { rotateCamera = b; });
    rotate->set_tooltip("rotate camera on the zx-plane");
    rotate->set_font_size(16);
    rotate->set_checked(rotateCamera);
    auto resetRotation = new Button(wrapper, "reset rotation");
    resetRotation->set_callback([&]() -> void { cameraTime = 0.0f; });
    resetRotation->set_font_size(16);
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

void CameraControls::setCameraParameters(const CameraParameters& params)
{
    this->params = params;
    refresh();
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
