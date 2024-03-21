#pragma once

#include "vk_types.h"

namespace vkutil {
    bool loader_shader_module(const char* file_path, VkDevice device, VkShaderModule* out_shader_module);
}

