#include <gui/gl_view.h>

#include <gui/camera_controls.h>
#include <render/camera.h>
#include <render/scene.h>
#include <string>

#include <nanogui/opengl.h>

using namespace nanogui;

using namespace std::string_literals;

GLView::GLView(Widget* parent, Toolbar* toolbar, const Scene& scene,
               ref<CameraControls> cameraControls)
    : Canvas{parent}, axesMesh{"../meshes/Axis.obj"},
      axesShader{Instance{axesMesh, {Material::Diffuse{::Color{0.7f}}}, {}}}, cameraControls{
                                                                                  cameraControls}
{
    meshDisplayControls = new Widget(toolbar);
    meshDisplayControls->set_layout(
        new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 4));

    (new Widget(meshDisplayControls))->set_fixed_width(8); // spacer
    auto icon = new Label(meshDisplayControls, utf8(FA_DESKTOP));
    icon->set_font("icons");
    new Label(meshDisplayControls, "3D View");
    (new Widget(meshDisplayControls))->set_fixed_width(8); // spacer

    auto renderMode = new ComboBox(meshDisplayControls, {"Depth", "Position", "Normal",
                                                         "My Little Shader", "Wobble", "Shaded"});
    renderMode->set_callback([&](int i) -> void { params.mode = MeshShader::RenderMode{i}; });
    renderMode->set_selected_index(static_cast<int>(params.mode));
    renderMode->set_font_size(16);
    renderMode->set_side(Popup::Down);

    new CheckBox(meshDisplayControls, "Wireframe", [&](bool b) -> void { params.wireframe = b; });
    new CheckBox(meshDisplayControls, "rotate point light",
                 [&](bool b) -> void { params.rotatePointLight = b; });
    /*
    new CheckBox(meshDisplayControls, "Show Axes", [&](bool b) -> void { params.showAxes = b; });
    new Label(meshDisplayControls, "BVH Level");
    auto bvhControl = new IntBox<int16_t>(meshDisplayControls, params.bvhLevel);
    bvhControl->set_callback([&](int16_t i) -> void { params.bvhLevel = i; });
    bvhControl->set_spinnable(true);
    bvhControl->set_min_max_values(0, BVH::maxDepth);
    bvhControl->set_font_size(16);
    bvhControl->set_fixed_width(60);
    */

    const Vector2i shadowRes{static_cast<int32_t>(shadowResolution.x),
                             static_cast<int32_t>(shadowResolution.y)};
    shadowTexture =
        new Texture(Texture::PixelFormat::Depth, Texture::ComponentFormat::Float32, shadowRes,
                    Texture::InterpolationMode::Bilinear, Texture::InterpolationMode::Bilinear,
                    Texture::WrapMode::ClampToEdge, 1,
                    Texture::TextureFlags::RenderTarget | Texture::TextureFlags::ShaderRead, true);
    glBindTexture(GL_TEXTURE_2D, shadowTexture->texture_handle());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    shadowPass = new RenderPass({}, shadowTexture);
    shadowPass->set_cull_mode(RenderPass::CullMode::Back);
    shadowPass->set_clear_depth(1.0f);
    shadowPass->set_depth_test(RenderPass::DepthTest::Less, true);

    setScene(scene);
}

void GLView::setScene(const Scene& scene)
{
    meshes.clear();
    bvhs.clear();

    for (const Instance& instance : scene.getInstances()) {
        meshes.emplace_back(instance);
        bvhs.emplace_back(instance);
    }
    for (const Light& light : scene.getLights()) {
        if (light.isPoint())
            params.pointLight = light.point();
    }

    sceneAABB = scene.getBounds();
}

void GLView::draw_contents()
{
    const float time = static_cast<float>(glfwGetTime());

    const Resolution res{static_cast<uint32_t>(m_size.x()), static_cast<uint32_t>(m_size.y())};
    const CameraParameters& cameraParams = cameraControls->getCameraParameters(res);

    MeshDisplayParameters params = this->params;
    if (params.rotatePointLight) {
        // rotate the point light
        float time = static_cast<float>(glfwGetTime());
        float sin = std::sin(time);
        float cos = std::cos(time);
        params.pointLight.pos.x =
            0.8f * (this->params.pointLight.pos.x * cos + this->params.pointLight.pos.z * sin);
        params.pointLight.pos.z =
            0.8f * (-this->params.pointLight.pos.x * sin + this->params.pointLight.pos.z * cos);
    }

    const Matrix4D view = cameraParams.viewTransformation();
    const Matrix4D proj = cameraParams.projection();
    const Matrix4D vp = proj * view;

    backgroundShader.draw({static_cast<float>(m_size.x()), static_cast<float>(m_size.y())});

    if (params.mode == MeshShader::RenderMode::ToyShader) {
        toyShader.draw(time);
        return;
    }

    if (params.showAxes) {
        axesShader.model = view;
        axesShader.draw(params.pointLight, vp);
    }

    render_pass()->set_cull_mode(RenderPass::CullMode::Disabled);
    glPolygonMode(GL_FRONT_AND_BACK, params.wireframe ? GL_LINE : GL_FILL);

    if (params.mode == MeshShader::RenderMode::Shaded) {
        for (auto& mesh : meshes)
            mesh.draw(params.pointLight, vp);
    }
    else if (params.mode == MeshShader::RenderMode::Wobble) {
        for (auto& mesh : meshes)
            mesh.drawWobble(params.pointLight, vp, time);
    }
    else {
        for (auto& mesh : meshes)
            mesh.drawDebug(sceneAABB, cameraParams.pos, params.mode, vp);
    }

    if (params.bvhLevel)
        for (auto& bvh : bvhs)
            bvh.draw(params.bvhLevel, vp);

    render_pass()->set_cull_mode(RenderPass::CullMode::Back);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
