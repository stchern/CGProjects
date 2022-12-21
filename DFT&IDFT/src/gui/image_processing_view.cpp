#include <gui/image_processing_view.h>

#include <misc/image_processing.h>

#include <iostream>
#include <stdexcept>
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
uniform bool fourier;
uniform bool flip_y;

// BEGIN TuroboColormap
// Copyright 2019 Google LLC.
// SPDX-License-Identifier: Apache-2.0

// Polynomial approximation in GLSL for the Turbo colormap
// Original LUT: https://gist.github.com/mikhailov-work/ee72ba4191942acecc03fe6da94fc73f

// Authors:
//   Colormap Design: Anton Mikhailov (mikhailov@google.com)
//   GLSL Approximation: Ruofei Du (ruofei@google.com)

vec3 TurboColormap(in float x) {
  const vec4 kRedVec4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
  const vec4 kGreenVec4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
  const vec4 kBlueVec4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
  const vec2 kRedVec2 = vec2(-152.94239396, 59.28637943);
  const vec2 kGreenVec2 = vec2(4.27729857, 2.82956604);
  const vec2 kBlueVec2 = vec2(-89.90310912, 27.34824973);

  x = min(1.0f, x);
  vec4 v4 = vec4( 1.0, x, x * x, x * x * x);
  vec2 v2 = v4.zw * v4.z;
  return vec3(
    dot(v4, kRedVec4)   + dot(v2, kRedVec2),
    dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
    dot(v4, kBlueVec4)  + dot(v2, kBlueVec2)
  );
}
// END TuroboColormap

void main() {
    vec2 frac = position_background - floor(position_background);
    float checkerboard = ((frac.x > .5) == (frac.y > .5)) ? 0.4 : 0.5;

    vec4 background = (1.0 - background_color.a) * vec4(vec3(checkerboard), 1.0) +
                              background_color.a * vec4(background_color.rgb, 1.0);

    if (fourier) {
        vec4 value = texture(image, uv+0.5);
        float amplitude = length(value.rg);
        frag_color = (1.0 - value.a) * background + vec4(TurboColormap(log2(1.0f+amplitude)), value.a);
    }
    else {
        vec4 value = texture(image, flip_y ? vec2(uv.x, 1.0f-uv.y) : uv);
        frag_color = (1.0 - value.a) * background + vec4(value.rrr, value.a);
    }
}
)"s;

DFTView::DFTView(nanogui::Widget* parent, Toolbar* toolbar) : nanogui::ImageView{parent}
{
    using namespace nanogui;

    // re-define image view shader (using log-scale display of red channel only)
    {
        m_image_shader = new Shader(render_pass(), "imageview_shader", vertex_shader_imageview,
                                    fragment_shader_imageview, Shader::BlendMode::None);

        const float positions[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};

        m_image_shader->set_buffer("position", VariableType::Float32, {6, 2}, positions);
        set_background_color(nanogui::Color{0.0f, 0.0f, 0.0f, 1.0f});
        m_image_shader->set_uniform("fourier", false);
        m_image_shader->set_uniform("flip_y", false);
    }

    dftControls = new Widget(toolbar);
    dftControls->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 4));

    (new Widget(dftControls))->set_fixed_width(8); // spacer
    auto icon = new Label(dftControls, utf8(FA_IMAGE));
    icon->set_font("icons");
    new Label(dftControls, "Discrete Fourier Transform");
    (new Widget(dftControls))->set_fixed_width(16); // spacer

    (new Button(dftControls, "load file", FA_FOLDER_OPEN))->set_callback([&]() -> void {
        try {
            const std::string filename = file_dialog({{"png", "Portable Network Graphics"},
                                                      {"jpg", "Joint Photographic Experts Group"},
                                                      {"jpeg", "Joint Photographic Experts Group"},
                                                      {"gif", "Graphics Interchange Format"},
                                                      {"ppm", "Portable PixMap"},
                                                      {"pgm", "Portable GrayMap"}},
                                                     false);
            if (!filename.empty()) {
                ::Texture image{filename, false};
                image = {image, ::Texture::Channels::R, ::Texture::DataType::Float};
                image = {image, ::Texture::Channels::RG, ::Texture::DataType::Float};
                setImage(image);
            }
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    });
    (new Button(dftControls, "DFT", FA_ARROW_CIRCLE_RIGHT))->set_callback([&]() -> void {
        next->setImage(DiscreteFourierTransform::dft(image), true);
    });
    (new Button(dftControls, "IDFT", FA_ARROW_CIRCLE_RIGHT))->set_callback([&]() -> void {
        next->setImage(DiscreteFourierTransform::dft(image, true));
    });
}

void DFTView::setImage(const Texture& image, bool fourierDisplay)
{
    using nanogui::Vector2i;

    this->image = image;

    nanogui::Texture::ComponentFormat componentFormat = nanogui::Texture::ComponentFormat::UInt8;
    switch (image.dataType) {
    case Texture::DataType::Float:
        componentFormat = nanogui::Texture::ComponentFormat::Float32;
        break;
    case Texture::DataType::UInt8:
        componentFormat = nanogui::Texture::ComponentFormat::UInt8;
        break;
    }

    nanogui::Texture::PixelFormat pixelFormat = nanogui::Texture::PixelFormat::RGBA;
    switch (image.channels) {
    case Texture::Channels::R:
        pixelFormat = nanogui::Texture::PixelFormat::R;
        break;
    case Texture::Channels::RG:
        pixelFormat = nanogui::Texture::PixelFormat::RA;
        break;
    case Texture::Channels::RGB:
        pixelFormat = nanogui::Texture::PixelFormat::RGB;
        break;
    case Texture::Channels::RGBA:
        pixelFormat = nanogui::Texture::PixelFormat::RGBA;
        break;
    }

    Vector2i res{static_cast<int32_t>(image.resolution.x),
                 static_cast<int32_t>(image.resolution.y)};

    texture = new nanogui::Texture{pixelFormat,
                                   componentFormat,
                                   res,
                                   nanogui::Texture::InterpolationMode::Trilinear,
                                   nanogui::Texture::InterpolationMode::Nearest,
                                   nanogui::Texture::WrapMode::Repeat};

    texture->upload(image.getData<uint8_t>().data());

    set_image(texture);
    this->fourierDisplay = fourierDisplay;
    m_image_shader->set_uniform("fourier", fourierDisplay);
    set_scale(std::min(static_cast<float>(m_size.x()) / static_cast<float>(res.x()),
                       static_cast<float>(m_size.y()) / static_cast<float>(res.y())));
    center();

    if (image.dataType == Texture::DataType::Float) {
        set_pixel_callback([this](const Vector2i& index, char** out, size_t size) {
            const auto& pixels = this->image.getData<float>();
            for (uint8_t ch = 0; ch < 4; ++ch) {
                if (ch < static_cast<uint8_t>(this->image.channels)) {
                    size_t x = static_cast<size_t>(index.x());
                    size_t y = static_cast<size_t>(index.y());
                    if (this->fourierDisplay) {
                        x = (x + this->image.resolution.x / 2) % this->image.resolution.x;
                        y = (y + this->image.resolution.y / 2) % this->image.resolution.y;
                    }
                    const size_t i = x + y * static_cast<size_t>(this->image.resolution.x);
                    const float value = pixels[i * static_cast<uint8_t>(this->image.channels) + ch];
                    snprintf(out[ch], size, "%f", value);
                }
                else
                    out[ch][0] = '\0';
            }
        });
    }
    else if (image.dataType == Texture::DataType::UInt8) {
        set_pixel_callback([this](const Vector2i& index, char** out, size_t size) {
            const auto& pixels = this->image.getData<uint8_t>();
            for (uint8_t ch = 0; ch < 4; ++ch) {
                if (ch < static_cast<uint8_t>(this->image.channels)) {
                    size_t x = static_cast<size_t>(index.x());
                    size_t y = static_cast<size_t>(index.y());
                    if (this->fourierDisplay) {
                        x = (x + this->image.resolution.x / 2) % this->image.resolution.x;
                        y = (y + this->image.resolution.y / 2) % this->image.resolution.y;
                    }
                    const size_t i = x + y * static_cast<size_t>(this->image.resolution.x);
                    const uint8_t value =
                        pixels[i * static_cast<uint8_t>(this->image.channels) + ch];
                    snprintf(out[ch], size, "%u", value);
                }
                else
                    out[ch][0] = '\0';
            }
        });
    }
}

SamplingView::SamplingView(nanogui::Widget* parent, Toolbar* toolbar) : nanogui::ImageView(parent)
{
    using namespace nanogui;

    // re-define image view shader (using red channel only)
    {
        m_image_shader = new Shader(render_pass(), "imageview_shader", vertex_shader_imageview,
                                    fragment_shader_imageview, Shader::BlendMode::None);

        const float positions[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};

        m_image_shader->set_buffer("position", VariableType::Float32, {6, 2}, positions);
        set_background_color(nanogui::Color{0.0f, 0.0f, 0.0f, 1.0f});
        m_image_shader->set_uniform("fourier", false);
        m_image_shader->set_uniform("flip_y", true);
    }

    samplingControls = new Widget(toolbar);
    samplingControls->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 4));

    (new Widget(samplingControls))->set_fixed_width(8); // spacer
    auto icon = new Label(samplingControls, utf8(FA_IMAGE));
    icon->set_font("icons");
    new Label(samplingControls, "Sampling");
    (new Widget(samplingControls))->set_fixed_width(16); // spacer

    auto samplingFunction =
        new ComboBox(samplingControls, {"Campbell-Robson", "Smooth Zone Plate", "Siemens Star"});
    samplingFunction->set_callback([&](int i) -> void {
        sampler.function = SamplingPatterns::Function{i};
        updateImage();
    });
    samplingFunction->set_selected_index(static_cast<int>(sampler.function));
    samplingFunction->set_font_size(16);
    samplingFunction->set_side(Popup::Down);

    new Label(samplingControls, "SPP");
    auto spp = new IntBox<uint32_t>(samplingControls, numSamplesPerDim);
    spp->set_callback([&, spp](uint32_t i) -> void {
        uint32_t samplesPerDim = static_cast<uint32_t>(std::round(std::sqrt(i)));

        if (i > samplesPerDim * samplesPerDim)
            ++samplesPerDim;
        else if (i < samplesPerDim * samplesPerDim)
            --samplesPerDim;

        numSamplesPerDim = samplesPerDim;
        spp->set_value(samplesPerDim * samplesPerDim);
        updateImage();
    });
    spp->set_min_value(1);
    spp->set_font_size(16);
    spp->set_fixed_width(60);
    spp->set_alignment(TextBox::Alignment::Right);
    spp->set_editable(true);
    spp->set_spinnable(true);

    auto samplingStrategy = new ComboBox(samplingControls, {"Regular", "Random", "Stratified"});
    samplingStrategy->set_callback([&](int i) -> void {
        sampler.samplingStrategy = SamplingPatterns::SamplingStrategy{i};
        updateImage();
    });
    samplingStrategy->set_selected_index(static_cast<int>(sampler.samplingStrategy));
    samplingStrategy->set_font_size(16);
    samplingStrategy->set_side(Popup::Down);

    set_pixel_callback([this](const Vector2i& index, char** out, size_t size) {
        const auto& pixels = image.getData<float>();
        for (uint8_t ch = 0; ch < 4; ++ch) {
            if (ch < 3) {
                size_t x = static_cast<size_t>(index.x());
                // flip y
                size_t y = image.resolution.y - static_cast<size_t>(index.y()) - 1;
                const size_t i = x + y * static_cast<size_t>(image.resolution.x);
                // always output channel 0
                const float value = pixels[i * static_cast<uint8_t>(image.channels) + 0];
                snprintf(out[ch], size, "%f", value);
            }
            else
                out[ch][0] = '\0';
        }
    });

    updateImage();
}

void SamplingView::updateImage()
{
    image = sampler.sample(res, numSamplesPerDim);
    texture = new nanogui::Texture{nanogui::Texture::PixelFormat::R,
                                   nanogui::Texture::ComponentFormat::Float32,
                                   {static_cast<int32_t>(res.x), static_cast<int32_t>(res.y)},
                                   nanogui::Texture::InterpolationMode::Trilinear,
                                   nanogui::Texture::InterpolationMode::Nearest};

    texture->upload(image.getData<uint8_t>().data());
    set_image(texture);
    center();
}
