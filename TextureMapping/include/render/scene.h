#ifndef SCENE_H
#define SCENE_H

#include "instance.h"
#include "light.h"
#include "material.h"
#include <geometry/matrix3d.h>
#include <geometry/mesh.h>
#include <geometry/point3d.h>

#include <vector>

class Scene {
public:
    /// add a mesh instance to the scene
    void addInstance(const Instance& instance)
    {
        bounds += instance.getBounds();
        instances.push_back(instance);
        if (instance.material.isEmitter())
            lights.push_back(Light{Light::Area{instance}});
    }

    /// add a pointlight to the scene
    void addPointLight(const Light::Point& light) { lights.push_back(Light{light}); }

    const std::vector<Instance>& getInstances() const { return instances; }
    const std::vector<Light>& getLights() const { return lights; }

    AABB getBounds() const { return bounds; }

    std::vector<Instance> instances;
    std::vector<Light> lights;
    AABB bounds;
};

#endif // !SCENE_H
