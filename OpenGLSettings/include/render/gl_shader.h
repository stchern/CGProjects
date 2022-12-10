#ifndef GL_SHADER_H
#define GL_SHADER_H

#include "gl_utils.h"
#include "light.h"

struct Instance;

class BackgroundShader final {
public:
    BackgroundShader();
    void draw(Point2D framebufferSize);

private:
    GLShaderProgram backgroundShader;
    GLVertexBuffer vertexBuffer{};
};

class BVHShader final {
public:
    BVHShader(const Instance& instance);
    void draw(uint32_t bvhLevel, const Matrix4D& vp);

    /// toWorld transformation matrix
    Matrix4D model;

private:
    size_t numBVHNodes{0};
    GLShaderProgram bvhShader;
    GLVertexBuffer vertexBuffer{};
};

class MeshShader final {
public:
    enum class RenderMode { Depth, Position, Normal, ToyShader, Wobble, Shaded };

    MeshShader(const Instance& instance);
    void draw(const Light::Point& light, const Matrix4D& vp);
    void drawWobble(const Light::Point& light, const Matrix4D& vp, float time);
    void drawDebug(const AABB& bounds, const Point3D& cameraPos, RenderMode mode,
                   const Matrix4D& vp);

    /// toWorld transformation matrix
    Matrix4D model;

private:
    std::vector<std::pair<size_t, size_t>> smoothGroups;
    size_t numTriangles{0};

    Material material;

    GLShaderProgram meshShader;
    GLShaderProgram wobbleShader;
    GLShaderProgram debugShader;
    GLVertexBuffer vertexBuffer{};
};

class MyLittleShader final {
public:
    MyLittleShader();
    ~MyLittleShader();

    MyLittleShader(const MyLittleShader&) = delete;
    MyLittleShader& operator=(const MyLittleShader&) = delete;
    MyLittleShader(MyLittleShader&&) = delete;
    MyLittleShader& operator=(MyLittleShader&&) = delete;

    void draw(float time);

private:
    /// the shader program
    GLuint program{};
    /// the vertex array representing the mapping of buffers to the shader program parameters
    GLuint vertexArray{};
    /// some buffers to work with
    std::array<GLuint, 4> buffers{};
};

#endif // GL_SHADER_H
