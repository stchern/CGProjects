#ifndef TEXTURE_H
#define TEXTURE_H

#include <geometry/point2d.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct Pixel {
    uint32_t x{}, y{};
    operator Point2D() const { return {static_cast<float>(x), static_cast<float>(y)}; }
    bool operator==(const Pixel& other) const = default;
};

using Resolution = Pixel;

struct Texture {
    enum class DataType { UInt8, Float };
    enum class Channels { R = 1, RG = 2, RGB = 3, RGBA = 4 };

    Resolution resolution{};
    Channels channels{Channels::RGBA};
    DataType dataType{DataType::Float};
    std::shared_ptr<std::byte> data{nullptr};

    /// create an empty (unusable) texture
    Texture() = default;
    /// create a new texture
    Texture(Resolution resolution, Channels channels, DataType dataType, std::byte* data = nullptr);
    /// load a texture from a file
    Texture(std::string_view filename, bool flip = true);
    /// convert from a different texture format
    Texture(const Texture& texture, Channels newChannels, DataType newDataType);

    /// returns true if this texture holds some data
    operator bool() const { return data.get(); }

    template <typename T> std::span<T> getData()
    {
        return {reinterpret_cast<T*>(data.get()), dataSize() / sizeof(T)};
    }
    template <typename T> std::span<const T> getData() const
    {
        return {reinterpret_cast<T*>(data.get()), dataSize() / sizeof(T)};
    }

    size_t dataSize() const
    {
        const size_t numValues = resolution.x * resolution.y * static_cast<size_t>(channels);
        if (dataType == DataType::Float)
            return numValues * sizeof(float);
        else
            return numValues * sizeof(uint8_t);
    }
};

#endif // TEXTURE_H
