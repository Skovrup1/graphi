#pragma once

#include "vk_descriptors.h"
#include "vk_types.h"

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push_func(std::function<void()> &&func) { deletors.push_back(func); }

    // functions callbacks are inefficient but this will be ok for now
    void flush() {
        for (auto iter = deletors.rbegin(); iter != deletors.rend(); iter++) {
            // call the function
            (*iter)();
        }

        deletors.clear();
    }
};

struct FrameData {
    VkCommandPool command_pool;
    VkCommandBuffer main_command_buffer;
    VkSemaphore swapchain_semaphore;
    VkSemaphore render_semaphore;
    VkFence render_fence;
    DeletionQueue deletion_queue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
  public:
    bool is_init{false};
    bool resize_requested{false};
    int frame_num{0};
    bool stop_rendering{false};
    DeletionQueue main_deletion_queue;
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
    VmaAllocator alloc;
    AllocactedImg draw_img;
    VkExtent2D draw_extent;
    DescriptorAllocator global_descriptor_allocator;
    VkDescriptorSet draw_img_descriptors;
    VkDescriptorSetLayout draw_img_descriptor_layout;
    VkPipeline gradient_pipeline;
    VkPipelineLayout gradient_pipeline_layout;
    VkFence imm_fence;
    VkCommandBuffer imm_command_buffer;
    VkCommandPool imm_command_pool;

    static VulkanEngine &Get();
    void init();
    void cleanup();
    void draw();
    void run();
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

  private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();
    void init_descriptors();
    void init_pipelines();
    void init_background_pipelines();
    void init_imgui();
    void resize_swapchain();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
    void draw_background(VkCommandBuffer cmd);
    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
};
