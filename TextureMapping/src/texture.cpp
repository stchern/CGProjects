#include <render/texture.h>

#include <iostream>
#include <stb_image.h>
#include <stdexcept>
#include <string>

using namespace std::string_literals;

const Texture& detail::TextureRegistry::loadTexture(std::string_view filename)
{
    TextureRegistry& instance = getInstance();
    if (!instance.textures.contains(filename.data())) {
        int x, y, imageChannels;
        stbi_uc* dataPtr = stbi_load(filename.data(), &x, &y, &imageChannels, 0);
        if (!dataPtr)
            throw std::runtime_error("failed to read image file "s + std::string(filename));

        const Texture texture{{static_cast<uint32_t>(x), static_cast<uint32_t>(y)},
                              Texture::Channels{imageChannels},
                              Texture::DataType{Texture::DataType::UInt8}};

        const size_t rowSize = texture.dataSize() / texture.resolution.y;
        // flip the texture vertically
        for (uint32_t i = 0; i < texture.resolution.y; ++i) {
            std::copy_n(reinterpret_cast<std::byte*>(dataPtr)
                            + rowSize * (texture.resolution.y - i - 1),
                        rowSize, texture.data + rowSize * i);
        }

        instance.textures.emplace(filename, texture);

        std::cout << "Loaded image file: " << filename << " with " << x << "x" << y
                  << " pixels and " << imageChannels << " channel(s)." << std::endl;
    }

    return instance.textures.at(filename.data());
}

void detail::TextureRegistry::allocateData(Texture& texture)
{
    std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(texture.dataSize());
    texture.data = buffer.get();
    getInstance().textureData.emplace_back(std::move(buffer));
}

Texture::Texture(Resolution resolution, Channels channels, DataType dataType, std::byte* data)
    : resolution{resolution}, channels{channels}, dataType{dataType}, data{data}
{
    if (!data && dataSize())
        detail::TextureRegistry::allocateData(*this);
}

Texture::Texture(std::string_view filename)
    : Texture{detail::TextureRegistry::loadTexture(filename)}
{
}
