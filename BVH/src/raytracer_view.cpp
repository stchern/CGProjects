#include "raytracer_view.h"

using namespace nanogui;

void RayTracerView::draw(NVGcontext* context)
{
    if (!texture || m_size != texture->size()) {
        texture =
            new Texture{Texture::PixelFormat::RGBA, Texture::ComponentFormat::Float32, m_size,
                        Texture::InterpolationMode::Trilinear, Texture::InterpolationMode::Nearest};
        set_image(texture);

        restart();
    }

    if (rayTracer->imageHasChanged()) {
        const auto& pixels = rayTracer->getFilm().getPixels();
        texture->upload(reinterpret_cast<const uint8_t*>(pixels.data()));
    }

    ImageView::draw(context);
}

void RayTracerView::restart()
{
    if (rayTracer) {
        const CameraParameters& cameraParams =
            cameraControls ? cameraControls->getCameraParameters(m_size) : CameraParameters{};
        rayTracer->resize(m_size, cameraParams);
    }
}
