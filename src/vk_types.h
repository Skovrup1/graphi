#pragma once

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

#include "vma/include/vk_mem_alloc.h"

#define VK_CHECK(x)                                                            \
    do {                                                                       \
        VkResult err = x;                                                      \
        if (err) {                                                             \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err));     \
            abort();                                                           \
        }                                                                      \
    } while (0)

struct AllocactedImg {
    VkImage img;
    VkImageView img_view;
    VmaAllocation allocation;
    VkExtent3D img_extent;
    VkFormat img_format;
};
