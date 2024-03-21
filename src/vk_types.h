#pragma once

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vulkan/vulkan.h>
#include "vma/include/vk_mem_alloc.h"

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define VK_CHECK(x)                                                            \
    do {                                                                       \
        VkResult err = x;                                                      \
        if (err) {                                                             \
            fmt::println("Detected Vulkan error: {}. Line: {}", string_VkResult(err), __LINE__);     \
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
