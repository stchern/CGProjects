#include <gui/gl_view.h>

#include <gui/camera_controls.h>
#include <render/camera.h>
#include <render/scene.h>
#include <string>

#include <nanogui/opengl.h>

using namespace std::string_literals;

GLView::GLView(nanogui::Widget* parent, Toolbar* toolbar, const Scene& scene,
               nanogui::ref<CameraControls> cameraControls)
    : Canvas{parent}, axesMesh{"../meshes/Axis.obj"},
      axesShader{Instance{axesMesh, {Material::Diffuse{::Color{0.7f}}}, {}}}, cameraControls{
                                                                                  cameraControls}
{
    using namespace nanogui;

    meshDisplayControls = new Widget(toolbar);
    meshDisplayControls->set_layout(
        new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 4));

    (new Widget(meshDisplayControls))->set_fixed_width(8); // spacer
    auto icon = new Label(meshDisplayControls, utf8(FA_DESKTOP));
    icon->set_font("icons");
    new Label(meshDisplayControls, "3D View");
    (new Widget(meshDisplayControls))->set_fixed_width(8); // spacer

    auto renderMode =
        new ComboBox(meshDisplayControls, {"My Little Shader", "Depth", "Position", "Normal", "UV",
                                           "Shaded", "Shadow Map", "Wobble"});
    renderMode->set_callback(
        [&](int i) -> void { params.meshShaderParameters.mode = MeshShader::RenderMode{i}; });
    renderMode->set_selected_index(static_cast<int>(params.meshShaderParameters.mode));
    renderMode->set_font_size(16);
    renderMode->set_side(Popup::Down);

    (new CheckBox(meshDisplayControls, "Wireframe", [&](bool b) -> void {
        params.wireframe = b;
    }))->set_checked(params.wireframe);
    (new CheckBox(meshDisplayControls, "rotate point light", [&](bool b) -> void {
        params.rotatePointLight = b;
    }))->set_checked(params.rotatePointLight);
    (new CheckBox(meshDisplayControls, "normal map", [&](bool b) -> void {
        params.meshShaderParameters.normalMap = b;
    }))->set_checked(params.meshShaderParameters.normalMap);
    (new CheckBox(meshDisplayControls, "displacement map", [&](bool b) -> void {
        params.meshShaderParameters.displacementMap = b;
    }))->set_checked(params.meshShaderParameters.displacementMap);
    (new CheckBox(meshDisplayControls, "shadow map", [&](bool b) -> void {
        params.meshShaderParameters.shadowMap = b;
    }))->set_checked(params.meshShaderParameters.shadowMap);
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

    setScene(scene);

    render_pass()->set_cull_mode(RenderPass::CullMode::Disabled);
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

    Light::Point pointLight = params.pointLight;
    if (params.rotatePointLight) {
        // rotate the point light
        float sin = std::sin(time);
        float cos = std::cos(time);
        pointLight.pos.x = 0.8f * (params.pointLight.pos.x * cos + params.pointLight.pos.z * sin);
        pointLight.pos.z = 0.8f * (-params.pointLight.pos.x * sin + params.pointLight.pos.z * cos);
    }

    const CameraParameters lightCameraParams{
        pointLight.pos,
        sceneAABB.center()
            + cameraParams.up * dot(cameraParams.up, (sceneAABB.min - sceneAABB.center())) * 0.5f,
        cameraParams.computeFrame().dir,
        {90.0f},
        {},
        shadowResolution,
        cameraParams.tNear,
        distance(cameraParams.pos, cameraParams.target) * 5.0f,
        CameraParameters::CameraType::Perspective};

    const Matrix4D lp = lightCameraParams.projection() * lightCameraParams.viewTransformation();
    const Matrix4D view = cameraParams.viewTransformation();
    const Matrix4D proj = cameraParams.projection();
    const Matrix4D vp = proj * view;

    if (params.meshShaderParameters.mode == MeshShader::RenderMode::ShadowMap) {
        // render the shadow map's view
        for (auto& mesh : meshes)
            mesh.drawShadow(lp);
        return;
    }

    if (params.meshShaderParameters.mode == MeshShader::RenderMode::Shaded
        && params.meshShaderParameters.shadowMap) {
        render_pass()->end();
        // render the shadow map
        shadowFramebuffer.bind();
        for (auto& mesh : meshes)
            mesh.drawShadow(lp);
        shadowFramebuffer.unbind();
        render_pass()->begin();
    }

    backgroundShader.draw({static_cast<float>(m_size.x()), static_cast<float>(m_size.y())});

    if (params.meshShaderParameters.mode == MeshShader::RenderMode::ToyShader) {
        toyShader.draw(time);
        return;
    }

    if (params.showAxes) {
        axesShader.model = view;
        axesShader.draw(pointLight, cameraParams.pos, vp, lp, {}, {});
    }

    glPolygonMode(GL_FRONT_AND_BACK, params.wireframe ? GL_LINE : GL_FILL);

    if (params.meshShaderParameters.mode == MeshShader::RenderMode::Shaded) {
        for (auto& mesh : meshes)
            mesh.draw(pointLight, cameraParams.pos, vp, lp, shadowFramebuffer.getDepthTexture(),
                      params.meshShaderParameters);
    }
    else if (params.meshShaderParameters.mode == MeshShader::RenderMode::Wobble) {
        for (auto& mesh : meshes)
            mesh.drawWobble(pointLight, vp, time);
    }
    else {
        for (auto& mesh : meshes)
            mesh.drawDebug(sceneAABB, cameraParams.pos, params.meshShaderParameters.mode, vp);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (params.bvhLevel)
        for (auto& bvh : bvhs)
            bvh.draw(static_cast<uint32_t>(params.bvhLevel), vp);
}
