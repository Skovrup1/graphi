#include "vk_init.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

VkCommandPoolCreateInfo
vkinit::command_pool_create_info(uint32_t queue_family_index,
                                 VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queue_family_index;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    return info;
}

VkCommandBufferAllocateInfo
vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    return info;
}

VkCommandBufferBeginInfo
vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;
    info.pInheritanceInfo = nullptr;
    info.flags = flags;

    return info;
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;

    return info;
}

VkSemaphoreCreateInfo
vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;

    return info;
}

VkImageSubresourceRange
vkinit::image_subresource_range(VkImageAspectFlags aspect_mask) {
    VkImageSubresourceRange sub_img{};
    sub_img.aspectMask = aspect_mask;
    sub_img.baseMipLevel = 0;
    sub_img.levelCount = VK_REMAINING_MIP_LEVELS;
    sub_img.baseArrayLayer = 0;
    sub_img.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return sub_img;
}

VkSemaphoreSubmitInfo
vkinit::semaphore_submit_info(VkPipelineStageFlags2 stage_mask,
                              VkSemaphore semaphore) {
    VkSemaphoreSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.semaphore = semaphore;
    submit_info.stageMask = stage_mask;
    submit_info.deviceIndex = 0;
    submit_info.value = 1;

    return submit_info;
}

VkCommandBufferSubmitInfo
vkinit::command_buffer_submit_info(VkCommandBuffer cmd) {
    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBuffer = cmd;
    info.deviceMask = 0;

    return info;
}

VkSubmitInfo2 vkinit::submit_info(VkCommandBufferSubmitInfo *cmd,
                                  VkSemaphoreSubmitInfo *signal_semaphore_info,
                                  VkSemaphoreSubmitInfo *wait_semaphore_info) {
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;
    if (wait_semaphore_info == nullptr) {
        info.waitSemaphoreInfoCount = 0;
    } else {
        info.waitSemaphoreInfoCount = 1;
    }
    info.pWaitSemaphoreInfos = wait_semaphore_info;

    if (signal_semaphore_info == nullptr) {
        info.signalSemaphoreInfoCount = 0;
    } else {
        info.signalSemaphoreInfoCount = 1;
    }
    info.pSignalSemaphoreInfos = signal_semaphore_info;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd;

    return info;
}
