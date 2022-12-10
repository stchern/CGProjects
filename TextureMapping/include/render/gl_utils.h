#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <nanogui/opengl.h>

#include "color.h"
#include "texture.h"
#include <geometry/matrix3d.h>
#include <geometry/matrix4d.h>
#include <geometry/mesh.h>
#include <geometry/point2d.h>
#include <geometry/point3d.h>

#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

void checkGL(const char* file, const int line);
#define CHECK_GL() checkGL(__FILE__, __LINE__)

class GLVertexBuffer {
public:
    GLVertexBuffer() = default;
    ~GLVertexBuffer()
    {
        if (!buffers.empty())
            glDeleteBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
        CHECK_GL();
    }
    // not copyable (but movable)
    GLVertexBuffer(const GLVertexBuffer&) = delete;
    GLVertexBuffer& operator=(const GLVertexBuffer&) = delete;
    GLVertexBuffer(GLVertexBuffer&& other) noexcept { swap(std::move(other)); }
    GLVertexBuffer& operator=(GLVertexBuffer&& other) noexcept
    {
        swap(std::move(other));
        return *this;
    }

    template <typename T,
              GLenum target = std::is_unsigned_v<T> ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER,
              std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
    void setBuffer(const std::string_view name, const std::span<const T>& values)
    {
        const auto it = std::find(bufferNames.begin(), bufferNames.end(), name);
        GLuint buffer;
        if (it == bufferNames.end()) {
            bufferNames.emplace_back(name);
            glGenBuffers(1, &buffer);
            buffers.push_back(buffer);
        }
        else
            buffer = buffers.at(static_cast<size_t>(std::distance(bufferNames.begin(), it)));

        glBindBuffer(target, buffer);
        const GLsizeiptr size = static_cast<GLsizeiptr>(values.size() * sizeof(T));
        glBufferData(target, size, values.data(), GL_STATIC_DRAW);
        glBindBuffer(target, 0);
    }

    template <typename T, std::enable_if_t<!std::is_fundamental_v<T>, int> = 0>
    void setBuffer(const std::string_view name, const std::span<const T>& values)
    {
        if (values.empty()) {
            std::cerr << "Warning: Input to buffer \"" << name << "\" is empty." << std::endl;
            return;
        }

        if constexpr (
            std::is_same_v<
                T,
                Point2D> || std::is_same_v<T, Point3D> || std::is_same_v<T, Point4D> || std::is_same_v<T, Color>)
            setBuffer<float>(name, {reinterpret_cast<const float*>(values.data()),
                                    values.size() * sizeof(T) / sizeof(float)});
        else if constexpr (std::is_same_v<T, TriangleIndices> || std::is_same_v<T, LineIndices>)
            setBuffer<uint32_t>(name, {reinterpret_cast<const uint32_t*>(values.data()),
                                       values.size() * sizeof(T) / sizeof(uint32_t)});
        else
            throw std::logic_error("unhandled data type");
    }

    void bind(const std::string_view name) const { glBindBuffer(GL_ARRAY_BUFFER, buffer(name)); }

    static void unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

    /// draw using indices (glDrawElements) GL_POINTS, GL_LINES, GL_TRIANGLES, GL_PATCHES
    void draw(const std::string_view name, GLenum mode, size_t from, size_t to) const;

private:
    GLuint buffer(const std::string_view name) const
    {
        const auto it = std::find(bufferNames.begin(), bufferNames.end(), name);
        if (it == bufferNames.end())
            throw std::runtime_error("buffer not found");
        return buffers.at(static_cast<size_t>(std::distance(bufferNames.begin(), it)));
    }

    /// vertex buffer objects
    std::vector<GLuint> buffers{};
    std::vector<std::string> bufferNames{};

    void swap(GLVertexBuffer&& other)
    {
        std::swap(buffers, other.buffers);
        std::swap(bufferNames, other.bufferNames);
    }
};

class GLShaderProgram {
public:
    GLShaderProgram(const std::string_view vertexShader, const std::string_view fragmentShader,
                    const std::string_view geometryShader = {},
                    const std::string_view tessControlShader = {},
                    const std::string_view tessEvalShader = {})
        : program{compileShaderProgram(vertexShader, fragmentShader, geometryShader,
                                       tessControlShader, tessEvalShader)}
    {
        glGenVertexArrays(1, &vertexArray);
        CHECK_GL();
    }
    ~GLShaderProgram()
    {
        glDeleteProgram(program);
        glDeleteVertexArrays(1, &vertexArray);
        CHECK_GL();
    }
    // not copyable (but movable)
    GLShaderProgram(const GLShaderProgram&) = delete;
    GLShaderProgram& operator=(const GLShaderProgram&) = delete;
    GLShaderProgram(GLShaderProgram&& other) noexcept { swap(std::move(other)); }
    GLShaderProgram& operator=(GLShaderProgram&& other) noexcept
    {
        swap(std::move(other));
        return *this;
    }

    /// set a uniform parameter of this shader program
    template <typename T> void setUniform(const std::string_view name, const T& value)
    {
        ensureActive();

        const GLint id = glGetUniformLocation(program, name.data());
        if (id == -1)
            std::cerr << "Warning: Uniform parameter \"" << name << "\" could not be not found.\n"
                      << "It might have been optimized away, if it is not used by the shader."
                      << std::endl;
        else if constexpr (std::is_same_v<T, GLfloat>)
            glUniform1f(id, value);
        else if constexpr (std::is_same_v<T, GLint> || std::is_same_v<T, bool>)
            glUniform1i(id, value);
        else if constexpr (std::is_same_v<T, GLuint>)
            glUniform1ui(id, value);
        else if constexpr (std::is_same_v<T, Color>)
            glUniform3f(id, value.r, value.g, value.b);
        else if constexpr (std::is_same_v<T, Point3D>)
            glUniform3f(id, value.x, value.y, value.z);
        else if constexpr (std::is_same_v<T, Point2D>)
            glUniform2f(id, value.x, value.y);
        else if constexpr (std::is_same_v<T, Matrix3D>)
            glUniformMatrix3fv(id, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&value));
        else if constexpr (std::is_same_v<T, Matrix4D>)
            glUniformMatrix4fv(id, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&value));
        else
            throw std::logic_error("unhandled uniform type");

        CHECK_GL();
    }

    /// set a vertex attribute to the currently selected buffer
    template <typename T>
    void setVertexAttribute(const std::string_view name, GLVertexBuffer& buffer)
    {
        ensureActive();

        buffer.bind(name);

        const GLuint location = static_cast<GLuint>(glGetAttribLocation(program, name.data()));
        if (location == -1U) {
            std::cerr << "Warning: Vertex attribute \"" << name << "\" could not be not found.\n"
                      << "It might have been optimized away, if it is not used in the shader."
                      << std::endl;
            return;
        }
        glEnableVertexAttribArray(location);
        if constexpr (std::is_same_v<T, GLfloat>)
            glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, 0, 0);
        else if constexpr (std::is_same_v<T, GLint>)
            glVertexAttribPointer(location, 1, GL_INT, GL_FALSE, 0, 0);
        else if constexpr (std::is_same_v<T, GLuint>)
            glVertexAttribPointer(location, 1, GL_UNSIGNED_INT, GL_FALSE, 0, 0);
        else if constexpr (std::is_same_v<T, Point2D>)
            glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, 0);
        else if constexpr (std::is_same_v<T, Point3D>)
            glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, 0);
        else if constexpr (std::is_same_v<T, Color>)
            glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, 0, 0);
        else
            throw std::logic_error("unhandled vertex attribute type");

        buffer.unbind();

        CHECK_GL();
    }

    /// enable this shader program
    void activate() const
    {
        glUseProgram(program);
        glBindVertexArray(vertexArray);
    }
    /// disable this shader program
    void deactivate() const
    {
        ensureActive();
        glBindVertexArray(0);
        glUseProgram(0);
    }

    static GLuint compileShaderProgram(const std::string_view vertexShader,
                                       const std::string_view fragmentShader,
                                       const std::string_view geometryShader = {},
                                       const std::string_view tessControlShader = {},
                                       const std::string_view tessEvalShader = {});

private:
    /// shader program handle
    GLuint program{};

    /// vertex array handle containing the parameters to this shader
    GLuint vertexArray{};

    /// ensure that the shader program is currently active
    void ensureActive() const
    {
        GLuint currentProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&currentProgram));
        if (currentProgram != program)
            throw std::runtime_error("please activate the shader program first");
    }

    void swap(GLShaderProgram&& other)
    {
        std::swap(program, other.program);
        std::swap(vertexArray, other.vertexArray);
    }
};

class GLTexture {
public:
    GLTexture(const Texture& image)
    {
        glGenTextures(1, &texture);

        const GLsizei resX = static_cast<GLsizei>(image.resolution.x);
        const GLsizei resY = static_cast<GLsizei>(image.resolution.y);

        GLenum imageFormat = GL_RGBA;
        switch (image.channels) {
        case Texture::Channels::R:
            imageFormat = GL_RED;
            break;
        case Texture::Channels::RG:
            imageFormat = GL_RG;
            break;
        case Texture::Channels::RGB:
            imageFormat = GL_RGB;
            break;
        case Texture::Channels::RGBA:
            imageFormat = GL_RGBA;
            break;
        }
        GLenum dataType = GL_UNSIGNED_BYTE;
        switch (image.dataType) {
        case Texture::DataType::UInt8:
            dataType = GL_UNSIGNED_BYTE;
            break;
        case Texture::DataType::Float:
            dataType = GL_FLOAT;
            break;
        }
        const GLint internalFormat = static_cast<GLint>(imageFormat);

        bind();
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, resX, resY, 0, imageFormat, dataType,
                     image.data);
        CHECK_GL();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);
        CHECK_GL();

        unbind();
    }
    ~GLTexture() { glDeleteTextures(1, &texture); }

    /// create an empty texture
    GLTexture(Resolution res, GLenum format = GL_RGBA)
    {
        if (res.x * res.y == 0)
            return;

        glGenTextures(1, &texture);

        const GLsizei resX = static_cast<GLsizei>(res.x);
        const GLsizei resY = static_cast<GLsizei>(res.y);

        const GLenum dataType = GL_FLOAT;

        bind();
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), resX, resY, 0, format, dataType,
                     nullptr);
        CHECK_GL();

        if (format == GL_DEPTH_COMPONENT) { // shadow map
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            const float white[4]{1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        CHECK_GL();

        unbind();
    }

    // unusable empty texture
    GLTexture() = default;

    // not copyable (but movable)
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;
    GLTexture(GLTexture&& other) noexcept { swap(std::move(other)); }
    GLTexture& operator=(GLTexture&& other) noexcept
    {
        swap(std::move(other));
        return *this;
    }

    void bind() const { glBindTexture(GL_TEXTURE_2D, texture); }

    static void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

    void computeMipMaps()
    {
        ensureActive();
        glGenerateMipmap(GL_TEXTURE_2D);
    }

private:
    /// texture name
    GLuint texture{};

    /// ensure that the texture is currently bound
    void ensureActive() const
    {
        GLuint currentTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&currentTexture));
        if (currentTexture != texture)
            throw std::runtime_error("please bind the texture first");
    }

    void swap(GLTexture&& other) { std::swap(texture, other.texture); }

    friend class GLFramebuffer;
};

class GLFramebuffer {
public:
    GLFramebuffer(Resolution res, bool color = true, bool depth = true)
        : resolution{res}, colorTarget{color ? res : Resolution{}, GL_RGBA},
          depthTarget{depth ? res : Resolution{}, GL_DEPTH_COMPONENT}
    {
        if (!color && !depth)
            throw std::logic_error("at least one texture needs to be created for this framebuffer");

        glGenFramebuffers(1, &framebuffer);

        bind(false, false);

        if (color)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   colorTarget.texture, 0);

        if (depth)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                   depthTarget.texture, 0);

        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("the framebuffer is incomplete");

        unbind(false);
    }
    ~GLFramebuffer() { glDeleteFramebuffers(1, &framebuffer); }

    // not copyable (but movable)
    GLFramebuffer(const GLTexture&) = delete;
    GLFramebuffer& operator=(const GLTexture&) = delete;
    GLFramebuffer(GLFramebuffer&& other) noexcept { swap(std::move(other)); }
    GLFramebuffer& operator=(GLFramebuffer&& other) noexcept
    {
        swap(std::move(other));
        return *this;
    }

    void bind(bool saveState = true, bool clear = true)
    {
        if (saveState)
            backup.saveState();

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        if (clear) {
            GLbitfield mask = 0;
            if (colorTarget.texture) {
                mask |= GL_COLOR_BUFFER_BIT;
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            }
            if (depthTarget.texture) {
                mask |= GL_DEPTH_BUFFER_BIT;
                glClearDepth(1.0f);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glDepthMask(GL_TRUE);
            }
            else
                glDisable(GL_DEPTH_TEST);

            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);

            glViewport(0, 0, static_cast<GLsizei>(resolution.x),
                       static_cast<GLsizei>(resolution.y));

            glClear(mask);

            CHECK_GL();
        }
    }

    const GLTexture& getColorTexture(bool computeMipMaps = true)
    {
        if (!colorTarget.texture)
            throw std::logic_error("this framebuffer does not have a color texture");

        if (computeMipMaps) {
            colorTarget.bind();
            colorTarget.computeMipMaps();
            colorTarget.unbind();
        }
        return colorTarget;
    }

    const GLTexture& getDepthTexture()
    {
        if (!depthTarget.texture)
            throw std::logic_error("this framebuffer does not have a depth texture");

        return depthTarget;
    }

    void unbind(bool restoreState = true)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (restoreState)
            backup.restoreState();
    }

private:
    /// framebuffer name
    GLuint framebuffer{};

    Resolution resolution{};
    GLTexture colorTarget{};
    GLTexture depthTarget{};

    /// most of the global OpenGL state we might want to change when switching framebuffers
    struct GLState {
        GLint viewport[4];
        GLboolean depthTest, depthMask, scissorTest, cullFace, blend;
        GLint cullMode;

        void saveState()
        {
            glGetIntegerv(GL_VIEWPORT, viewport);
            depthTest = glIsEnabled(GL_DEPTH_TEST);
            glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
            scissorTest = glIsEnabled(GL_SCISSOR_TEST);
            cullFace = glIsEnabled(GL_CULL_FACE);
            glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
            blend = glIsEnabled(GL_BLEND);
        }

        void restoreState()
        {
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            if (depthTest)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
            glDepthMask(depthMask);
            if (scissorTest)
                glEnable(GL_SCISSOR_TEST);
            else
                glDisable(GL_SCISSOR_TEST);
            if (cullFace)
                glEnable(GL_CULL_FACE);
            else
                glDisable(GL_CULL_FACE);
            glCullFace(static_cast<GLenum>(cullMode));
            if (blend)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);
        }
    } backup{};

    /// ensure that the render pass is currently bound
    void ensureActive() const
    {
        GLuint currentFramebuffer;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&currentFramebuffer));
        if (currentFramebuffer != framebuffer)
            throw std::runtime_error("please bind the framebuffer first");
    }

    void swap(GLFramebuffer&& other) { std::swap(framebuffer, other.framebuffer); }
};

#endif // GL_UTILS_H
