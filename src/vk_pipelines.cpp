#include "vk_pipelines.h"
#include <fstream>

bool vkutil::loader_shader_module(const char *file_path, VkDevice device,
                                  VkShaderModule *out_shader_module) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    size_t file_size = (size_t)file.tellg();

    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

    file.seekg(0);

    file.read((char*)buffer.data(), file_size);

    file.close();

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.codeSize = buffer.size() * sizeof(uint32_t);
    create_info.pCode = buffer.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        return false;
    }

    // output parameter
    *out_shader_module = shader_module;

    return true;
}
