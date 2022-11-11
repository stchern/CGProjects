#include "mesh.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>

void Mesh::loadOBJ(const std::string& filename)
{
    clear();

    std::ifstream file{filename};
    file.exceptions(std::ios::badbit);
    std::string buffer;

    if (!file)
        throw std::runtime_error(
            std::string{"failed to open the OBJ file "} + filename
            + std::string{"\nmake sure you run the program in the correct folder!"});

    auto check = [&]() -> void {
        if (!file) {
            file.close();
            clear();
            throw std::runtime_error(std::string{"failed to parse the OBJ file "} + filename
                                     + "\ncurrent line:\n" + buffer);
        }
    };

    std::vector<Vertex> objNormals;
    std::vector<TextureCoordinate> objTexCoords;
    std::vector<std::optional<TriangleIndices>> normalIndices, textureIndices;

    size_t currentSmoothGroup = 0;

    while (file) {
        std::getline(file, buffer);

        if (buffer.empty() || buffer.starts_with("#"))
            continue;

        std::istringstream in{buffer};
        char type, subtype;
        in >> std::noskipws >> type >> subtype >> std::skipws;

        if (type == 'v') {
            if (subtype == 't') {
                TextureCoordinate vt;
                in >> vt.x >> vt.y;
                check();

                objTexCoords.push_back(vt);
            }
            else if (subtype == ' ' || subtype == '\t' || subtype == 'n') {
                Vertex v;
                in >> v.x >> v.y >> v.z;
                check();

                if (subtype == 'n') {
                    objNormals.push_back(v);
                }
                else {
                    aabb.extend(v);
                    vertices.push_back(v);
                }
            }
        }
        else if (type == 'f') {
            TriangleIndices t, tn, tt;

            bool hasNormal{true}, hasTexture{true};

            auto readIndices = [&](uint32_t& v, uint32_t& vt, uint32_t& vn) -> void {
                in >> v;
                --v;

                if (in) {
                    // texture coordinate and vertex normal id (currently unused)
                    if (in.peek() == '/') {
                        in.ignore();
                        if (in.peek() != '/') {
                            in >> vt;
                            if (in)
                                --vt;
                            else
                                hasTexture = false;
                        }
                        else
                            hasTexture = false;
                        if (in.peek() == '/') {
                            in.ignore();
                            in >> vn;
                            if (in)
                                --vn;
                            else
                                hasNormal = false;
                        }
                        else
                            hasNormal = false;
                    }
                    else
                        hasNormal = hasTexture = false;
                    in.clear();
                }
            };

            readIndices(t.v1, tt.v1, tn.v1);
            readIndices(t.v2, tt.v2, tn.v2);
            readIndices(t.v3, tt.v3, tn.v3);
            check();

            while (in) {
                if (hasNormal) {
                    if (normalIndices.size() < faces.size())
                        normalIndices.resize(faces.size());
                    normalIndices.emplace_back(tn);
                }
                if (hasTexture) {
                    if (textureIndices.size() < faces.size())
                        textureIndices.resize(faces.size());
                    textureIndices.emplace_back(tt);
                }
                faces.push_back(t);

                t.v2 = t.v3;
                tn.v2 = tn.v3;
                tt.v2 = tt.v3;
                readIndices(t.v3, tt.v3, tn.v3);
            }
        }
        else if (type == 's') {
            // end current smooth group
            if (faces.size() > currentSmoothGroup)
                smoothGroups.emplace_back(currentSmoothGroup, faces.size());
            if (buffer.find("off") != buffer.npos)
                currentSmoothGroup = -1UL;
            else
                currentSmoothGroup = faces.size();
        }
    }

    if (faces.size() > currentSmoothGroup)
        smoothGroups.emplace_back(currentSmoothGroup, faces.size());

    file.close();

    // compute face areas, and normals (and texture coordinates, if any) per vertex
    {
        std::vector<float> weights(vertices.size());

        faceAreas.resize(faces.size());
        normals.resize(vertices.size());
        if (textureIndices.size())
            texCoords.resize(vertices.size());

        bool shadeFlat = true;
        auto currentSmoothGroup = smoothGroups.cbegin();

        for (size_t i = 0; i < faces.size(); ++i) {
            if (currentSmoothGroup != smoothGroups.end()) {
                if (i == currentSmoothGroup->first)
                    shadeFlat = false;
                if (i == currentSmoothGroup->second) {
                    shadeFlat = true;
                    ++currentSmoothGroup;
                }
            }

            const TriangleIndices& t = faces.at(i);
            const Vertex v1v2 = vertices.at(t.v2) - vertices.at(t.v1);
            const Vertex v1v3 = vertices.at(t.v3) - vertices.at(t.v1);
            const Vertex v2v3 = vertices.at(t.v3) - vertices.at(t.v2);
            const Vertex upTimes2Area = cross(v1v2, v1v3);
            const float faceArea = upTimes2Area.norm() * 0.5f;
            faceAreas.at(i) = faceArea;

            const Vertex v1v2n = normalize(v1v2);
            const Vertex v1v3n = normalize(v1v3);
            const Vertex v2v3n = normalize(v2v3);

            // weight = angle covered by the triangle
            const float w1 = std::abs(dot(v1v2n, v1v3n));
            const float w2 = std::abs(dot(v1v2n, v2v3n));
            const float w3 = std::abs(dot(v2v3n, v1v3n));

            weights.at(t.v1) += w1;
            weights.at(t.v2) += w2;
            weights.at(t.v3) += w3;

            // update factor for incremental mean, max(0, x) prevents nan caused by 0/0
            const float u1 = std::max(0.0f, (w1 / weights.at(t.v1)));
            const float u2 = std::max(0.0f, (w2 / weights.at(t.v2)));
            const float u3 = std::max(0.0f, (w3 / weights.at(t.v3)));

            if (i < normalIndices.size() && normalIndices.at(i)) {
                const TriangleIndices& tn = *normalIndices.at(i);
                normals.at(t.v1) += (objNormals.at(tn.v1) - normals.at(t.v1)) * u1;
                normals.at(t.v2) += (objNormals.at(tn.v2) - normals.at(t.v2)) * u2;
                normals.at(t.v3) += (objNormals.at(tn.v3) - normals.at(t.v3)) * u3;
            }
            else {
                const Vertex up = normalize(upTimes2Area);
                normals.at(t.v1) += (up - normals.at(t.v1)) * u1;
                if (!shadeFlat) {
                    normals.at(t.v2) += (up - normals.at(t.v2)) * u2;
                    normals.at(t.v3) += (up - normals.at(t.v3)) * u3;
                }
            }
            if (texCoords.size() > i && textureIndices.at(i)) {
                const TriangleIndices& tt = *textureIndices.at(i);
                texCoords.at(t.v1) += (objTexCoords.at(tt.v1) - texCoords.at(t.v1)) * u1;
                texCoords.at(t.v2) += (objTexCoords.at(tt.v2) - texCoords.at(t.v2)) * u2;
                texCoords.at(t.v3) += (objTexCoords.at(tt.v3) - texCoords.at(t.v3)) * u3;
            }
        }
    }

    std::cout << "Loaded OBJ file: " << filename << " containing " << vertices.size()
              << " vertices, " << objNormals.size() << " vertex normals, " << objTexCoords.size()
              << " texture coordinates, and " << faces.size() << " faces." << std::endl;

    bvh.construct(*this);
}

void Mesh::updateBounds()
{
    aabb = {};
    for (const auto& vertex : vertices) {
        aabb.extend(vertex);
    }
}
