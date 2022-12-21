#include <render/texture.h>

#include <iostream>
#include <stb_image.h>
#include <stdexcept>
#include <string>

using namespace std::string_literals;

Texture::Texture(Resolution resolution, Channels channels, DataType dataType, std::byte* data)
    : resolution{resolution}, channels{channels}, dataType{dataType}, data{data}
{
    if (!data && dataSize())
        this->data.reset(new std::byte[dataSize()]);
}

Texture::Texture(std::string_view filename, bool flip)
{
    int x, y, imageChannels;
    stbi_set_flip_vertically_on_load(flip);
    stbi_uc* dataPtr = stbi_load(filename.data(), &x, &y, &imageChannels, 0);
    if (!dataPtr)
        throw std::runtime_error("failed to read image file "s + std::string(filename));

    *this = {{static_cast<uint32_t>(x), static_cast<uint32_t>(y)},
             Texture::Channels{imageChannels},
             Texture::DataType{Texture::DataType::UInt8},
             reinterpret_cast<std::byte*>(dataPtr)};

    std::cout << "Loaded image file: " << filename << " with " << x << "x" << y << " pixels and "
              << imageChannels << " channel(s)." << std::endl;
}

Texture::Texture(const Texture& texture, Channels newChannels, DataType newDataType)
    : Texture{texture.resolution, newChannels, newDataType}
{
    if (!dataSize())
        return;

    const auto inputUint8Pixels = texture.getData<uint8_t>();
    const auto inputFloatPixels = texture.getData<float>();
    const uint8_t inputChannelCount = static_cast<uint8_t>(texture.channels);

    auto newUint8Pixels = getData<uint8_t>();
    auto newFloatPixels = getData<float>();
    const uint8_t newChannelCount = static_cast<uint8_t>(newChannels);

    for (uint32_t y = 0; y < resolution.y; ++y) {
        for (uint32_t x = 0; x < resolution.x; ++x) {
            for (uint8_t c = 0; c < newChannelCount; ++c) {
                float fValue = 0.0f;
                uint8_t uValue = 0;
                if (c < inputChannelCount) {
                    if (texture.dataType == DataType::Float) {
                        fValue = inputFloatPixels[(y * resolution.x + x) * inputChannelCount + c];
                        if (newDataType == DataType::UInt8)
                            uValue = static_cast<uint8_t>(
                                std::max(0.0f, std::min(255.0f, fValue * 255.0f)));
                    }
                    else if (texture.dataType == DataType::UInt8) {
                        uValue = inputUint8Pixels[(y * resolution.x + x) * inputChannelCount + c];
                        if (newDataType == DataType::Float)
                            fValue = static_cast<float>(uValue) / 255.0f;
                    }
                }
                if (newDataType == DataType::Float)
                    newFloatPixels[(y * resolution.x + x) * newChannelCount + c] = fValue;
                else if (newDataType == DataType::UInt8)
                    newUint8Pixels[(y * resolution.x + x) * newChannelCount + c] = uValue;
            }
        }
    }
}
