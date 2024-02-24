#pragma once

#include "vk_types.h"
#include <vector>
#include <vulkan/vulkan_core.h>

struct FrameData {
    VkCommandPool command_pool;
    VkCommandBuffer main_command_buffer;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
  public:
    bool is_init{false};
    int frame_num{0};
    bool stop_rendering{false};
    VkExtent2D window_extent{1700, 900};
    struct SDL_Window *window{nullptr};
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice active_gpu;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_img_format;
    std::vector<VkImage> swapchain_imgs;
    std::vector<VkImageView> swapchain_img_views;
    VkExtent2D swapchain_extent;
    FrameData frames[FRAME_OVERLAP];
    FrameData &get_current_frame() {
        return frames[frame_num % FRAME_OVERLAP];
    };
    VkQueue graphics_queue;
    uint32_t graphics_queue_family;

    static VulkanEngine &Get();
    void init();
    void cleanup();
    void draw();
    void run();

  private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
};
