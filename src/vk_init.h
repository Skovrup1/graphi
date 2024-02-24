#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace vkinit {
    VkCommandPoolCreateInfo
    command_pool_create_info(uint32_t queue_family_index,
                             VkCommandPoolCreateFlags flags);

    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool,
                                                             uint32_t count);
};
