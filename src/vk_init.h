#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace vkinit {
VkCommandPoolCreateInfo
command_pool_create_info(uint32_t queue_family_index,
                         VkCommandPoolCreateFlags flags);
VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool,
                                                         uint32_t count);
VkCommandBufferBeginInfo
command_buffer_begin_info(VkCommandBufferUsageFlags flags);
VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags);
VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags);
VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);
VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signal_semaphore, VkSemaphoreSubmitInfo* wait_semaphore_info);
}; // namespace vkinit
