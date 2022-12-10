#include <gui/raytracer_view.h>

#include <string>

using namespace std::literals::string_literals;

static const std::string vertex_shader_imageview = R"(
#version 330

uniform mat4 matrix_image;
uniform mat4 matrix_background;
in vec2 position;
out vec2 position_background;
out vec2 uv;

void main() {
    vec4 p = vec4(position, 0.0, 1.0);
    gl_Position = matrix_image * p;
    position_background = (matrix_background * p).xy;
    uv = position;
}
)"s;

static const std::string fragment_shader_imageview = R"(
#version 330

in vec2 uv;
in vec2 position_background;
out vec4 frag_color;
uniform sampler2D image;
uniform vec4 background_color;
uniform float blur;
uniform bool useSRGB;

void main() {
    vec2 frac = position_background - floor(position_background);
    float checkerboard = ((frac.x > .5) == (frac.y > .5)) ? 0.4 : 0.5;

    vec4 background = (1.0 - background_color.a) * vec4(vec3(checkerboard), 1.0) +
                              background_color.a * vec4(background_color.rgb, 1.0);

    vec4 value = texture(image, uv, blur);

    if (useSRGB) {
        vec3 srgb = vec3(
            value.r < 0.0031308 ? 12.92 * value.r : (1.055 * pow(value.r, 1.0/2.4) - 0.055),
            value.g < 0.0031308 ? 12.92 * value.g : (1.055 * pow(value.g, 1.0/2.4) - 0.055),
            value.b < 0.0031308 ? 12.92 * value.b : (1.055 * pow(value.b, 1.0/2.4) - 0.055)
        );

        frag_color = (1.0 - value.a) * background + vec4(srgb, value.a);
    }
    else
        frag_color = (1.0 - value.a) * background + value;
}
)"s;

using namespace nanogui;

RayTracerView::RayTracerView(Widget* parent, Toolbar* toolbar, RayTracer& rayTracer,
                             nanogui::ref<CameraControls> cameraControls)
    : ImageView(parent), rayTracer{rayTracer}, cameraControls{cameraControls}
{
    // re-define shader (using pre-multiplied alpha)
    {
        m_image_shader = new Shader(render_pass(), "imageview_shader", vertex_shader_imageview,
                                    fragment_shader_imageview, Shader::BlendMode::AlphaBlend);

        const float positions[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};

        m_image_shader->set_buffer("position", VariableType::Float32, {6, 2}, positions);
        m_image_shader->set_uniform("blur", 0.0f);
        m_image_shader->set_uniform("useSRGB", false);
    }

    rayTracerControls = new Widget(toolbar);
    rayTracerControls->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 4));

    (new Widget(rayTracerControls))->set_fixed_width(8); // spacer
    auto icon = new Label(rayTracerControls, utf8(FA_IMAGE));
    icon->set_font("icons");
    new Label(rayTracerControls, "Ray Tracer");
    (new Widget(rayTracerControls))->set_fixed_width(16); // spacer

    auto renderMode =
        new ComboBox(rayTracerControls, {"Depth", "Position", "Normal", "Whitted", "Path"});
    renderMode->set_callback(
        [&](int i) -> void { params.mode = RayTracerParameters::RenderMode{i}; });
    renderMode->set_selected_index(static_cast<int>(params.mode));
    renderMode->set_font_size(16);
    renderMode->set_side(Popup::Down);

    new Label(rayTracerControls, "max depth");
    auto maxDepth = new IntBox<uint16_t>(rayTracerControls, params.maxDepth);
    maxDepth->set_callback([&](uint16_t i) -> void { params.maxDepth = i; });
    maxDepth->set_font_size(16);
    maxDepth->set_fixed_width(40);
    maxDepth->set_alignment(TextBox::Alignment::Right);
    maxDepth->set_editable(true);
    maxDepth->set_spinnable(true);
    maxDepth->set_min_value(1);

    new Label(rayTracerControls, "SPP");
    auto spp = new IntBox<uint16_t>(rayTracerControls, params.maxSPP);
    spp->set_callback([&](uint16_t i) -> void { params.maxSPP = i; });
    spp->set_font_size(16);
    spp->set_fixed_width(60);
    spp->set_alignment(TextBox::Alignment::Right);
    spp->set_editable(true);
    spp->set_spinnable(true);
    spp->set_min_value(1);

    progress = new ProgressBar(rayTracerControls);

    auto resetView = new Button(rayTracerControls, "Reset View", FA_VECTOR_SQUARE);
    resetView->set_callback([&]() -> void { reset(); });
    resetView->set_font_size(16);

    set_pixel_callback([&](const Vector2i& index, char** out, size_t size) {
        const auto& pixels = rayTracer.getFilm().getPixels();
        for (uint8_t ch = 0; ch < 4; ++ch) {
            const size_t i =
                static_cast<size_t>(index.x())
                + static_cast<size_t>(index.y()) * static_cast<size_t>(texture->size().x());
            const float value = pixels.at(i)[ch];
            snprintf(out[ch], size, "%f", value);
        }
    });
}

void RayTracerView::draw(NVGcontext* context)
{
    if (!texture || m_size != texture->size()) {
        texture =
            new Texture{Texture::PixelFormat::RGBA, Texture::ComponentFormat::Float32, m_size,
                        Texture::InterpolationMode::Trilinear, Texture::InterpolationMode::Nearest};
        set_image(texture);
    }
    else if (rayTracer.imageHasChanged()) {
        const float sppRendered = rayTracer.getSPPRendererd();
        const auto& pixels = rayTracer.getFilm().getPixels();
        texture->upload(reinterpret_cast<const uint8_t*>(pixels.data()));

        progress->set_value(sppRendered / params.maxSPP);
        progress->set_tooltip(std::to_string(static_cast<uint32_t>(sppRendered)) + " / "s
                              + std::to_string(params.maxSPP));
        m_image_shader->set_uniform("blur",
                                    5.0f * std::exp(-sppRendered * (1.0f / std::log(5.0f))));
        m_image_shader->set_uniform("useSRGB",
                                    params.mode == RayTracerParameters::RenderMode::Whitted
                                        || params.mode == RayTracerParameters::RenderMode::Path);
    }
    ImageView::draw(context);

    const Resolution res{static_cast<uint32_t>(m_size.x()), static_cast<uint32_t>(m_size.y())};

    if (rayTracer.setParams(params, cameraControls->getCameraParameters(res)))
        rayTracer.start();
}
