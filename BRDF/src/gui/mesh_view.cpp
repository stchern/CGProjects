#include <gui/mesh_view.h>

#include <gui/camera_controls.h>
#include <render/camera.h>
#include <render/scene.h>
#include <string>

#include <nanogui/opengl.h>
#include <nanogui/vector.h>

using namespace nanogui;

using namespace std::string_literals;

static const std::string vertex_shader_checkers = R"(
#version 330

uniform mat4 matrix_image;
uniform mat4 matrix_background;
in vec2 position;
out vec2 position_background;
out vec2 uv;

void main() {
    vec4 p = vec4(position, 0.0, 1.0);
    gl_Position = p;
    position_background = (matrix_background * p).xy;
}

)"s;

static const std::string fragment_shader_checkers = R"(
#version 330

in vec2 position_background;
out vec4 frag_color;
uniform vec4 background_color;

void main() {
    vec2 frac = position_background - floor(position_background);
    float checkerboard = ((frac.x > .5) == (frac.y > .5)) ? 0.4 : 0.5;

    vec4 background = (1.0 - background_color.a) * vec4(vec3(checkerboard), 1.0) +
                              background_color.a * vec4(background_color.rgb, 1.0);

    frag_color = background;
}

)"s;

MeshView::MeshView(Widget* parent, Toolbar* toolbar, const Scene& scene,
                   nanogui::ref<CameraControls> cameraControls)
    : Canvas{parent}, axesMesh{"../meshes/Axis.obj"},
      axesShader{Instance{&axesMesh, {Material::Diffuse{::Color{0.7f}}}, {}}, render_pass()},
      cameraControls{cameraControls}
{
    meshDisplayControls = new Widget(toolbar);
    meshDisplayControls->set_layout(
        new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 4));

    (new Widget(meshDisplayControls))->set_fixed_width(8); // spacer
    auto icon = new Label(meshDisplayControls, utf8(FA_DESKTOP));
    icon->set_font("icons");
    new Label(meshDisplayControls, "3D View");
    (new Widget(meshDisplayControls))->set_fixed_width(8); // spacer

    new CheckBox(meshDisplayControls, "Wireframe", [&](bool b) -> void { params.wireframe = b; });
    new CheckBox(meshDisplayControls, "Normals", [&](bool b) -> void { params.normals = b; });
    new CheckBox(meshDisplayControls, "Show Axes", [&](bool b) -> void { params.showAxes = b; });
    new Label(meshDisplayControls, "BVH Level");
    auto bvhControl = new IntBox<int16_t>(meshDisplayControls, params.bvhLevel);
    bvhControl->set_callback([&](int16_t i) -> void { params.bvhLevel = i; });
    bvhControl->set_spinnable(true);
    bvhControl->set_min_max_values(0, BVH::maxDepth);
    bvhControl->set_font_size(16);
    bvhControl->set_fixed_width(60);

    // background shader
    backgroundShader = new Shader(render_pass(), "checker_shader", vertex_shader_checkers,
                                  fragment_shader_checkers);

    const float positions[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f};

    backgroundShader->set_buffer("position", VariableType::Float32, {6, 2}, positions);
    backgroundShader->set_uniform("background_color", nanogui::Color{0.f, 0.f, 0.f, 0.f});

    setScene(scene);
}

void MeshView::setScene(const Scene& scene)
{
    meshes.clear();
    for (const Instance& instance : scene.getInstances())
        meshes.emplace_back(instance, render_pass());
}

void MeshView::draw_contents()
{
    const Matrix4f matrixBackground =
        Matrix4f::scale(Vector3f(m_size.x() / 40.f, m_size.y() / 40.f, 1.f))
        * Matrix4f::translate(-Vector3f(m_size.x() / 40.f, m_size.y() / 40.f, 0.f));
    backgroundShader->set_uniform("matrix_background", Matrix4f(matrixBackground));
    backgroundShader->begin();
    render_pass()->set_depth_test(render_pass()->depth_test().first, false);
    backgroundShader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, false);
    render_pass()->set_depth_test(render_pass()->depth_test().first, true);
    backgroundShader->end();

    const Resolution res{static_cast<uint32_t>(m_size.x()), static_cast<uint32_t>(m_size.y())};

    const CameraParameters& cameraParams = cameraControls->getCameraParameters(res);

    Matrix4f view = Matrix4f::look_at(cameraParams.pos, cameraParams.target, cameraParams.up);

    Matrix4f proj =
        (cameraParams.type == CameraParameters::CameraType::Perspective)
            ? Matrix4f::perspective(cameraParams.perspective.fov * degToRad, 0.1f, 100.f,
                                    cameraParams.aspect())
            : Matrix4f::ortho(cameraParams.orthographic.left, cameraParams.orthographic.right,
                              cameraParams.orthographic.bottom, cameraParams.orthographic.top, 0.1f,
                              20.f);

    const Matrix4f vp = proj * view;

    if (params.showAxes) {
        axesShader.model = view;
        axesShader.draw(params, vp);
    }

    if (params.wireframe) {
        render_pass()->set_cull_mode(RenderPass::CullMode::Disabled);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
        render_pass()->set_cull_mode(RenderPass::CullMode::Back);

    for (auto& mesh : meshes)
        mesh.draw(params, vp);

    if (params.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
