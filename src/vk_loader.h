#pragma once

#include "vk_types.h"
#include <unordered_map>
#include <filesystem>

struct GeoSurface {
    uint32_t start_index;
    uint32_t count;
};

struct MeshAsset {
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
};

class VulkanEngine;

std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_gltf_meshes(VulkanEngine *engine, std::filesystem::path file_path);
