#ifndef FILM_H
#define FILM_H

#include "color.h"
#include <geometry/point2d.h>

#include <vector>

struct Pixel {
    uint32_t x, y;
    operator Point2D() const { return {static_cast<float>(x), static_cast<float>(y)}; }
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
        size_t index = pixelCoordinate.x + resolution.x * pixelCoordinate.y;
        if (index >= pixels.size())
            return;

        // numerically stable incremental mean
        pixels[index] += (color - pixels[index]) * (1.0f / ++pixelWeights[index]);
    }

    const Resolution getResolution() const { return resolution; }

    const std::vector<Color>& getPixels() const { return pixels; }

private:
    Resolution resolution;
    std::vector<Color> pixels;
    std::vector<float> pixelWeights;
};

#endif // !FILM_H
