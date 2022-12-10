#include <geometry/mesh.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>

using namespace std::string_literals;

void Mesh::loadOBJ(const std::string_view filename)
{
    clear();

    std::ifstream file{filename.data()};
    file.exceptions(std::ios::badbit);
    std::string buffer;

    if (!file)
        throw std::runtime_error("failed to open the OBJ file "s + filename.data()
                                 + "\nmake sure you run the program in the correct folder!"s);

    auto check = [&]() -> void {
        if (!file) {
            file.close();
            clear();
            throw std::runtime_error("failed to parse the OBJ file "s + filename.data()
                                     + "\ncurrent line:\n"s + buffer);
        }
    };

    std::vector<Normal3D> objNormals;
    std::vector<Point2D> objTexCoords;
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
                Point2D vt;
                in >> vt.x >> vt.y;
                check();

                objTexCoords.push_back(vt);
            }
            else if (subtype == ' ' || subtype == '\t' || subtype == 'n') {
                Point3D v;
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

    // duplicate some shared vertices
    {
        const bool hasTexCoords = !objTexCoords.empty();
        std::vector<std::optional<uint32_t>> texCoordPerVertex(hasTexCoords ? vertices.size()
                                                                            : 0UL);
        std::vector<bool> verticesUsedForFlatNormal(vertices.size());
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

            TriangleIndices& face = faces.at(i);

            // duplicate reused vertices if they have a different texture coordinate
            if (i < textureIndices.size() && textureIndices.at(i)) {
                const TriangleIndices& tt = *textureIndices.at(i);

                if (texCoordPerVertex.at(face.v1) && *texCoordPerVertex.at(face.v1) != tt.v1) {
                    const uint32_t copyIndex = vertices.size();
                    vertices.push_back(vertices.at(face.v1));
                    face.v1 = copyIndex;
                }
                else
                    texCoordPerVertex.at(face.v1) = tt.v1;

                if (texCoordPerVertex.at(face.v2) && *texCoordPerVertex.at(face.v2) != tt.v2) {
                    const uint32_t copyIndex = vertices.size();
                    vertices.push_back(vertices.at(face.v2));
                    face.v2 = copyIndex;
                }
                else
                    texCoordPerVertex.at(face.v2) = tt.v2;

                if (texCoordPerVertex.at(face.v3) && *texCoordPerVertex.at(face.v3) != tt.v3) {
                    const uint32_t copyIndex = vertices.size();
                    vertices.push_back(vertices.at(face.v3));
                    face.v3 = copyIndex;
                    continue;
                }
                else
                    texCoordPerVertex.at(face.v3) = tt.v3;
            }

            // duplicate reused vertices if they are used to set the normal for flat shading
            if (shadeFlat) {
                if (verticesUsedForFlatNormal.at(face.v3)) {
                    const uint32_t copyIndex = vertices.size();
                    vertices.push_back(vertices.at(face.v3));
                    face.v3 = copyIndex;
                    continue;
                }
            }
            verticesUsedForFlatNormal.at(face.v3) = true;
        }
    }

    // compute face areas, and normals (and texture coordinates, if any) per vertex
    {
        std::vector<float> weights(vertices.size());

        normals.resize(vertices.size());
        if (textureIndices.size())
            texCoords.resize(vertices.size());
        faceAreas.resize(faces.size());
        faceAreaPrefixSum.resize(faces.size());

        bool shadeFlat = true;
        auto currentSmoothGroup = smoothGroups.cbegin();
        float areaSum = 0.0f;

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
            Vector3D v1v2 = vertices.at(t.v2) - vertices.at(t.v1);
            Vector3D v1v3 = vertices.at(t.v3) - vertices.at(t.v1);
            Vector3D v2v3 = vertices.at(t.v3) - vertices.at(t.v2);
            const Vector3D upTimes2Area = cross(v1v2, v1v3);
            const float area = upTimes2Area.norm() * 0.5f;
            areaSum += area;
            faceAreaPrefixSum.at(i) = areaSum;
            faceAreas.at(i) = area;

            const Normal3D up = normalize(upTimes2Area);

            if (shadeFlat) {
                normals.at(t.v3) = up;
            }
            else {
                v1v2 = normalize(v1v2);
                v1v3 = normalize(v1v3);
                v2v3 = normalize(v2v3);

                // weight = angle covered by the triangle
                const float w1 = std::abs(dot(v1v2, v1v3));
                const float w2 = std::abs(dot(v1v2, v2v3));
                const float w3 = std::abs(dot(v2v3, v1v3));

                // update factor for incremental mean
                float u1 = weights.at(t.v1) > 0.0f ? 1.0f : w1;
                float u2 = weights.at(t.v2) > 0.0f ? 1.0f : w2;
                float u3 = weights.at(t.v3) > 0.0f ? 1.0f : w3;

                weights.at(t.v1) += w1;
                weights.at(t.v2) += w2;
                weights.at(t.v3) += w3;

                u1 = w1 > 0.0f ? w1 / weights.at(t.v1) : u1;
                u2 = w2 > 0.0f ? w2 / weights.at(t.v2) : u2;
                u3 = w3 > 0.0f ? w3 / weights.at(t.v3) : u3;

                if (i < normalIndices.size() && normalIndices.at(i)) {
                    const TriangleIndices& tn = *normalIndices.at(i);
                    normals.at(t.v1) += (objNormals.at(tn.v1) - normals.at(t.v1)) * u1;
                    normals.at(t.v2) += (objNormals.at(tn.v2) - normals.at(t.v2)) * u2;
                    normals.at(t.v3) += (objNormals.at(tn.v3) - normals.at(t.v3)) * u3;
                }
                else {
                    normals.at(t.v1) += (up - normals.at(t.v1)) * u1;
                    normals.at(t.v2) += (up - normals.at(t.v2)) * u2;
                    normals.at(t.v3) += (up - normals.at(t.v3)) * u3;
                }
            }
            if (i < textureIndices.size() && textureIndices.at(i)) {
                const TriangleIndices& tt = *textureIndices.at(i);
                // vertices with different texture coordinates have been duplicated before
                texCoords.at(t.v1) = objTexCoords.at(tt.v1);
                texCoords.at(t.v2) = objTexCoords.at(tt.v2);
                texCoords.at(t.v3) = objTexCoords.at(tt.v3);
            }
        }

        invTotalArea = 1.0f / areaSum;
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

std::pair<Point3D, Normal3D> Mesh::computePointAndNormal(uint32_t faceIndex,
                                                         const BarycentricCoordinates& bary) const
{
    const TriangleIndices& face = faces.at(faceIndex);
    const Triangle& triangle = getTriangleFromFace(face);
    const TriangleNormals& triangleNormals = getTriangleNormalsFromFace(face);

    const Point3D p = bary.interpolate(triangle);
    const Normal3D n = isSmoothFace(faceIndex) ? bary.interpolate(triangleNormals)
                                               : cross(triangle.v1v2, triangle.v1v3);

    return {p, n};
}

std::pair<Point3D, Normal3D> Mesh::samplePointAndNormal(Point2D sample) const
{
    // find the bin of this sample using binary search (meshes might be huge)
    const auto it = std::lower_bound(faceAreaPrefixSum.cbegin(), faceAreaPrefixSum.cend(),
                                     sample.x * faceAreaPrefixSum.back());
    const uint32_t faceIndex = static_cast<uint32_t>(std::distance(faceAreaPrefixSum.cbegin(), it));
    // reuse the sample
    {
        if (faceIndex)
            sample.x -= *(it - 1) * invTotalArea;
        sample.x /= *it * invTotalArea;
    }

    BarycentricCoordinates bary{sample.x, sample.y};
    if (bary.lambda2 + bary.lambda3 > 1.0f) {
        bary.lambda2 = 1.0f - bary.lambda2;
        bary.lambda3 = 1.0f - bary.lambda3;
    }

    return computePointAndNormal(faceIndex, bary);
}
