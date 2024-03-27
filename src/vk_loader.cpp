#include "vk_loader.h"
#include <iostream>
#include <stb_image.h>

#include "vk_engine.h"
#include "vk_init.h"
#include "vk_types.h"

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

std::optional<std::vector<std::shared_ptr<MeshAsset>>>
load_gltf_meshes(VulkanEngine *engine, std::filesystem::path file_path) {
    fmt::print("Loading GLTF: {}\n", file_path.string());

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(file_path);

    constexpr auto gltf_options = fastgltf::Options::LoadGLBBuffers |
                                  fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};

    auto load =
        parser.loadBinaryGLTF(&data, file_path.parent_path(), gltf_options);
    fmt::print("past\n");
    if (load) {
        gltf = std::move(load.get());
    } else {
        fmt::print("Failed to load glTF: {}\n",
                   fastgltf::to_underlying(load.error()));
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh &mesh : gltf.meshes) {
        MeshAsset new_mesh;

        new_mesh.name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto &&p : mesh.primitives) {
            GeoSurface new_surface;
            new_surface.start_index = (uint32_t)indices.size();
            new_surface.count =
                (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor &index_accessor =
                    gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + index_accessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(
                    gltf, index_accessor, [&](std::uint32_t idx) {
                        indices.push_back(idx + initial_vtx);
                    });
            }

            // load vertex positions
            {
                fastgltf::Accessor &pos_accessor =
                    gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + pos_accessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, pos_accessor, [&](glm::vec3 v, size_t index) {
                        Vertex new_vtx;
                        new_vtx.position = v;
                        new_vtx.normal = {1, 0, 0};
                        new_vtx.color = glm::vec4{1.f};
                        new_vtx.uv_x = 0;
                        new_vtx.uv_y = 0;
                        vertices[initial_vtx + index] = new_vtx;
                    });
            }

            // UVs
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load vertex colors
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vtx + index].uv_x = v.x;
                        vertices[initial_vtx + index].uv_y = v.y;
                    });
            }
            new_mesh.surfaces.push_back(new_surface);
        }

        // display vertex normals
        constexpr bool override_colors = true;
        if (override_colors) {
            for (Vertex &vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        new_mesh.mesh_buffers = engine->upload_mesh(indices, vertices);

        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(new_mesh)));
    }

    return meshes;
}
