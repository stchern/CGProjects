#ifndef FILM_H
#define FILM_H

#include "color.h"
#include <geometry/point2d.h>

#include <vector>

struct Pixel {
    uint32_t x, y;
    operator Point2D() const { return {static_cast<float>(x), static_cast<float>(y)}; }
    bool operator==(const Pixel& other) const = default;
};

using Resolution = Pixel;

/** This is a film class that holds the color buffer for the raytracer.
 */
class Film {
public:
    /** Constructor
     * @param resolution The image resolution.
     */
    Film(const Resolution& resolution = {})
        : resolution{(resolution.x > 0 && resolution.y > 0) ? resolution : Resolution{}},
          pixels(this->resolution.x * this->resolution.y),
          pixelWeights(this->resolution.x * this->resolution.y)
    {
    }

    /** Add a color to a pixel in the buffer.
     * @param pixel_coordinate The pixel that should be updated.
     * @param color The color that should be added.
     */
    void addPixelColor(Pixel pixelCoordinate, const Color& color)
    {
        const uint32_t index = pixelCoordinate.x + resolution.x * pixelCoordinate.y;
        if (index >= pixels.size())
            return;

        // numerically stable incremental mean
        pixels[index] += (color - pixels[index]) * (1.0f / ++pixelWeights[index]);
    }

    /**
     * @brief addPixelColorSplat adds a pixel value and splats it to neighboring pixels without adding any weight to them
     * @param pixelCoordinate
     * @param color
     * @param splat
     */
    void addPixelColorUnweighted(Pixel pixelCoordinate, const Color& color, int32_t splat)
    {
        for (int32_t y=-splat/2; y<(splat+1)/2; ++y) {
            const int32_t pixelY = pixelCoordinate.y+y;
            if (pixelY < 0 || pixelY >= static_cast<int32_t>(resolution.y))
                continue;
            for (int32_t x=-splat/2; x<(splat+1)/2; ++x) {
                const int32_t pixelX = pixelCoordinate.x+x;
                if (pixelX < 0 || pixelX >= static_cast<int32_t>(resolution.x))
                    continue;

                const uint32_t index = pixelX + resolution.x * pixelY;

                pixels[index] = color;

                if (x == 0 && y == 0)
                    pixelWeights[index] = 1;
            }
        }
    }

    const Resolution getResolution() const { return resolution; }

    const std::vector<Color>& getPixels() const { return pixels; }

    void clearWeights() { std::fill(pixelWeights.begin(), pixelWeights.end(), 0); }

private:
    Resolution resolution;
    std::vector<Color> pixels;
    std::vector<uint32_t> pixelWeights;
};

#endif // !FILM_H
