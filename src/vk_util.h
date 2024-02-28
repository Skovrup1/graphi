#pragma once

#include "vk_types.h"

namespace vkutil {
void transition_img(VkCommandBuffer cmd, VkImage img, VkImageLayout curr_layout,
                    VkImageLayout new_layout);
void copy_img_to_img(VkCommandBuffer cmd, VkImage src, VkImage dest,
                     VkExtent2D src_size, VkExtent2D dst_size);
}; // namespace vkutil
