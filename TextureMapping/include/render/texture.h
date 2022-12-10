#ifndef TEXTURE_H
#define TEXTURE_H

#include <geometry/point2d.h>
#include <render/color.h>

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
public:
    enum class DataType { UInt8, Float };
    enum class Channels { R = 1, RG = 2, RGB = 3, RGBA = 4 };

    Resolution resolution{};
    Channels channels{Channels::RGBA};
    DataType dataType{DataType::Float};
    /// non-owning pointer to the texture's data (all data is owned by the TextureRegistry class)
    std::byte* data{nullptr};

    /// create an empty (unusable) texture
    Texture() = default;
    /// create a new texture
    Texture(Resolution resolution, Channels channels, DataType dataType, std::byte* data = nullptr);
    /// load a texture from a file
    Texture(std::string_view filename);

    /// returns true if this texture holds some data
    operator bool() const { return data; }

    template <typename T> std::span<T> getData()
    {
        return {reinterpret_cast<T*>(data), dataSize() / sizeof(T)};
    }
    template <typename T> std::span<const T> getData() const
    {
        return {reinterpret_cast<T*>(data), dataSize() / sizeof(T)};
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

namespace detail {
class TextureRegistry {
public:
    static TextureRegistry& getInstance()
    {
        static TextureRegistry instance;
        return instance;
    }
    static const Texture& loadTexture(std::string_view filename);
    static void allocateData(Texture& texture);

private:
    TextureRegistry() = default;
    /// all textures that have been loaded by name
    std::map<std::string, Texture> textures;
    /// holds all texture data in the program
    std::vector<std::unique_ptr<std::byte[]>> textureData;
};
}; // namespace detail

#endif // TEXTURE_H
