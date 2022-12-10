#ifndef INSTANCE_H
#define INSTANCE_H

#include "material.h"
#include <geometry/matrix3d.h>
#include <geometry/mesh.h>
#include <geometry/point3d.h>

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace detail {
class MeshRegistry {
public:
    static MeshRegistry& getInstance()
    {
        static MeshRegistry instance;
        return instance;
    }
    static const Mesh* loadMesh(const std::string_view filename)
    {
        MeshRegistry& instance = getInstance();
        if (!instance.meshes.contains(filename.data()))
            instance.meshes.emplace(filename, std::make_unique<Mesh>(filename));
        return instance.meshes.at(filename.data()).get();
    }

private:
    MeshRegistry() = default;
    std::map<std::string, std::unique_ptr<Mesh>> meshes;
};
}; // namespace detail

struct Instance {
    Instance(const Mesh& mesh, const Material& material = {},
             const HomogeneousTransformation3D& toWorld = {})
        : mesh{mesh}, material{material}, toWorld{toWorld}
    {
    }
    Instance(const std::string_view meshFilename, const Material& material = {},
             const HomogeneousTransformation3D& toWorld = {})
        : Instance{*detail::MeshRegistry::loadMesh(meshFilename), material, toWorld}
    {
    }

    const Mesh& mesh;
    const Material material{Material::Diffuse{Color{165 / 255.0f, 30 / 255.0f, 55 / 255.0f}}};
    const HomogeneousTransformation3D toWorld{};
    const HomogeneousTransformation3D toLocal{toWorld.inverse()};
    const Matrix3D normalToWorld{toLocal.m.transposed()};

    AABB getBounds() const
    {
        return {toWorld * mesh.getBounds().min, toWorld * mesh.getBounds().max};
    }

    std::pair<Point3D, Normal3D> samplePointAndNormal(Point2D sample) const
    {
        auto [p, n] = mesh.samplePointAndNormal(sample);
        return {toWorld * p, normalize(normalToWorld * n)};
    }
};

#endif // INSTANCE_H
