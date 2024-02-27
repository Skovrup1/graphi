#pragma once

#include <vulkan/vulkan_core.h>

namespace vkutil {
void transition_img(VkCommandBuffer cmd, VkImage img, VkImageLayout curr_layout,
                    VkImageLayout new_layout);
};
