#ifndef FILM_H
#define FILM_H

#include "color.h"
#include "texture.h"
#include <geometry/point2d.h>

#include <span>
#include <vector>

/** This is a film class that holds the color buffer for the raytracer.
 */
class Film {
public:
    /** Constructor
     * @param resolution The image resolution.
     */
    Film(const Resolution& resolution = {})
        : texture{(resolution.x > 0 && resolution.y > 0) ? resolution : Resolution{},
                  Texture::Channels::RGBA, Texture::DataType::Float},
          pixelWeights(texture.resolution.x * texture.resolution.y)
    {
    }

    /** Add a color to a pixel in the buffer.
     * @param pixel_coordinate The pixel that should be updated.
     * @param color The color that should be added.
     */
    void addPixelColor(Pixel pixelCoordinate, const Color& color)
    {
        if (pixelCoordinate.x > texture.resolution.x || pixelCoordinate.y >= texture.resolution.y)
            return;

        const uint32_t index = pixelCoordinate.x + texture.resolution.x * pixelCoordinate.y;

        // numerically stable incremental mean
        getPixels()[index] +=
            (color - getPixels()[index]) * (1.0f / static_cast<float>(++pixelWeights[index]));
    }

    /**
     * @brief addPixelColorSplat adds a pixel value and splats it to neighboring pixels without
     * adding any weight to them
     * @param pixelCoordinate
     * @param color
     * @param splat
     */
    void addPixelColorUnweighted(Pixel pixelCoordinate, const Color& color, int32_t splat)
    {
        for (int32_t y = -splat / 2; y < (splat + 1) / 2; ++y) {
            const int32_t pixelY = static_cast<int32_t>(pixelCoordinate.y) + y;
            if (pixelY < 0 || pixelY >= static_cast<int32_t>(texture.resolution.y))
                continue;
            for (int32_t x = -splat / 2; x < (splat + 1) / 2; ++x) {
                const int32_t pixelX = static_cast<int32_t>(pixelCoordinate.x) + x;
                if (pixelX < 0 || pixelX >= static_cast<int32_t>(texture.resolution.x))
                    continue;

                const uint32_t index = static_cast<uint32_t>(pixelX)
                                     + static_cast<uint32_t>(pixelY) * texture.resolution.x;

                getPixels()[index] = color;

                if (x == 0 && y == 0)
                    pixelWeights[index] = 1;
            }
        }
    }

    const Resolution getResolution() const { return texture.resolution; }

    std::span<Color> getPixels() { return texture.getData<Color>(); }
    std::span<const Color> getPixels() const { return texture.getData<Color>(); }

    void clearWeights() { std::fill(pixelWeights.begin(), pixelWeights.end(), 0); }

private:
    Texture texture;
    std::vector<uint32_t> pixelWeights;
};

#endif // !FILM_H
