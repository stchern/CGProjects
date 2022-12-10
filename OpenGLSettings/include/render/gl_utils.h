#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <nanogui/opengl.h>

#include "color.h"
#include <geometry/matrix3d.h>
#include <geometry/matrix4d.h>
#include <geometry/mesh.h>
#include <geometry/point2d.h>
#include <geometry/point3d.h>

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

    /// draw using indices (glDrawElements)
    void draw(const std::string_view name, GLenum mode, size_t from, size_t to) const;

    /// draw using LineIndices
    void drawLines(const std::string_view name, size_t from, size_t to) const
    {
        draw(name, GL_LINES, from, to);
    }
    /// draw using TriangleIndices
    void drawTriangles(const std::string_view name, size_t from, size_t to) const
    {
        draw(name, GL_TRIANGLES, from, to);
    }

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
                    const std::string_view geometryShader = {})
        : program{compileShaderProgram(vertexShader, fragmentShader, geometryShader)}
    {
        glCreateVertexArrays(1, &vertexArray);
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
        if constexpr (std::is_same_v<T, GLfloat>)
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

    void setTexture(const std::string_view name, GLuint texture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        const GLint id = glGetUniformLocation(program, name.data());
        glUniform1i(id, 0);
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
                                       const std::string_view geometryShader = {});

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

#endif // GL_UTILS_H
