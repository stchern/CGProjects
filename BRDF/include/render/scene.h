#ifndef SCENE_H
#define SCENE_H

#include "light.h"
#include "material.h"
#include <geometry/matrix3d.h>
#include <geometry/mesh.h>
#include <geometry/point3d.h>

#include <map>
#include <memory>
#include <vector>

struct Instance {
    Instance() = default;
    Instance(const Mesh* mesh, const Material& material, const HomogeneousTransformation3D& toWorld)
        : mesh{mesh}, material{material}, toWorld{toWorld}
    {
    }

    const Mesh* mesh;
    const Material material{Material::Diffuse{Color{165 / 255.0f, 30 / 255.0f, 55 / 255.0f, 1.0f}}};
    const HomogeneousTransformation3D toWorld{};
    const HomogeneousTransformation3D toLocal{toWorld.inverse()};
    const Matrix3D normalToWorld{toLocal.m.transpose()};

    AABB getBounds() const
    {
        return {toWorld * mesh->getBounds().min, toWorld * mesh->getBounds().max};
    }
};

class Scene {
public:
    /// add a mesh instance to the scene
    void addInstance(Instance&& instance)
    {
        bounds += instance.getBounds();
        instances.emplace_back(std::move(instance));
    }
    void addInstance(const Mesh* mesh, Material material = {},
                     HomogeneousTransformation3D toWorld = {})
    {
        addInstance({mesh, material, toWorld});
    }
    void addInstance(const std::string& meshFilename, Material material = {},
                     HomogeneousTransformation3D toWorld = {})
    {
        addInstance({MeshRegistry::loadMesh(meshFilename), material, toWorld});
    }
    /// add a pointlight to the scene
    void addLight(const PointLight& light) { lights.push_back(light); }

    const std::vector<Instance>& getInstances() const { return instances; }
    const std::vector<PointLight>& getLights() const { return lights; }

    AABB getBounds() const { return bounds; }

private:
    class MeshRegistry {
    public:
        static MeshRegistry& getInstance()
        {
            static MeshRegistry instance;
            return instance;
        }
        static const Mesh* loadMesh(const std::string& filename)
        {
            MeshRegistry& instance = getInstance();
            if (!instance.meshes.contains(filename))
                instance.meshes.emplace(filename, std::make_unique<Mesh>(filename));
            return instance.meshes.at(filename).get();
        }

    private:
        MeshRegistry() = default;
        std::map<std::string, std::unique_ptr<Mesh>> meshes;
    };

    std::vector<Instance> instances;
    std::vector<PointLight> lights;
    AABB bounds;
};

#endif // !SCENE_H
