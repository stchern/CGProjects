#ifndef SCENE_H
#define SCENE_H

#include "mesh.h"

#include <vector>

class Scene {
public:
    void addMesh(const Mesh& mesh) { meshes.push_back(mesh); }
    void addMesh(Mesh&& mesh) { meshes.push_back(std::move(mesh)); }

    const std::vector<Mesh>& getMeshes() const { return meshes; }

private:
    std::vector<Mesh> meshes;
};

#endif // !SCENE_H
