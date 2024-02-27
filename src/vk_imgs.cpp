#include "vk_imgs.h"
#include "vk_init.h"
#include <vulkan/vulkan_core.h>

void vkutil::transition_img(VkCommandBuffer cmd, VkImage img, VkImageLayout curr_layout,
                    VkImageLayout new_layout) {
    VkImageMemoryBarrier2 img_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    img_barrier.pNext = nullptr;

    img_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    img_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    img_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    img_barrier.dstAccessMask =
        VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    img_barrier.oldLayout = curr_layout;
    img_barrier.newLayout = new_layout;

    VkImageAspectFlags aspect_mask;
    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    img_barrier.subresourceRange = vkinit::image_subresource_range(aspect_mask);
    img_barrier.image = img;

    VkDependencyInfo dep_info = {};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.pNext = nullptr;

    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &img_barrier;

    vkCmdPipelineBarrier2(cmd, &dep_info);
}
