#include "vk_engine.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vk-bootstrap/src/VkBootstrap.h"

#define VMA_IMPLEMENTATION
#include "vma/include/vk_mem_alloc.h"

#include "vk_types.h"
#include "vk_init.h"
#include "vk_util.h"
#include "vk_descriptors.h"
#include "vk_pipelines.h"

constexpr bool use_validation_layers = true;
VulkanEngine *loaded_engine = nullptr;

VulkanEngine &VulkanEngine::Get() { return *loaded_engine; }

void VulkanEngine::init() {
    assert(loaded_engine == nullptr);
    loaded_engine = this;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    window = SDL_CreateWindow("Graphi engine", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, window_extent.width,
                              window_extent.height, window_flags);
    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    init_descriptors();

    init_pipelines();

    is_init = true;
}

void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("graphi app")
                        .request_validation_layers(use_validation_layers)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();

    instance = vkb_inst.instance;
    debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(window, instance, &surface);

    // 1.3 features
    VkPhysicalDeviceVulkan13Features features{};
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // 1.2 features
    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physical_device =
        selector.set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
            .set_surface(surface)
            .select()
            .value();

    vkb::DeviceBuilder device_builder{physical_device};
    vkb::Device vkb_device = device_builder.build().value();

    device = vkb_device.device;
    active_gpu = physical_device.physical_device;

    graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    graphics_queue_family =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo alloc_info = {};
    alloc_info.physicalDevice = active_gpu;
    alloc_info.device = device;
    alloc_info.instance = instance;
    alloc_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&alloc_info, &alloc);

    main_deletion_queue.push_func([&]() { vmaDestroyAllocator(alloc); });
}

void VulkanEngine::init_swapchain() {
    create_swapchain(window_extent.width, window_extent.height);

    VkExtent3D draw_img_extent = {
        window_extent.width,
        window_extent.height,
        1,
    };

    draw_img.img_format = VK_FORMAT_R16G16B16A16_SFLOAT;
    draw_img.img_extent = draw_img_extent;

    VkImageUsageFlags draw_img_usages{};
    draw_img_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_img_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    draw_img_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
    draw_img_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::img_create_info(
        draw_img.img_format, draw_img_usages, draw_img_extent);

    VmaAllocationCreateInfo rimg_alloc_info = {};
    rimg_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_alloc_info.requiredFlags =
        VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(alloc, &rimg_info, &rimg_alloc_info, &draw_img.img,
                   &draw_img.allocation, nullptr);

    VkImageViewCreateInfo rview_info = vkinit::img_view_create_info(
        draw_img.img_format, draw_img.img, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(
        vkCreateImageView(device, &rview_info, nullptr, &draw_img.img_view));

    main_deletion_queue.push_func([*this]() {
        vkDestroyImageView(device, draw_img.img_view, nullptr);
        vmaDestroyImage(alloc, draw_img.img, draw_img.allocation);
    });
}

void VulkanEngine::resize_swapchain() {}

void VulkanEngine::init_commands() {
    VkCommandPoolCreateInfo command_pool_info =
        vkinit::command_pool_create_info(
            graphics_queue_family,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr,
                                     &frames[i].command_pool));

        VkCommandBufferAllocateInfo cmd_alloc_info =
            vkinit::command_buffer_allocate_info(frames[i].command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &cmd_alloc_info,
                                          &frames[i].main_command_buffer));
    }
}

void VulkanEngine::init_sync_structures() {
    VkFenceCreateInfo fence_create_info =
        vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphore_create_info =
        vkinit::semaphore_create_info(0);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr,
                               &frames[i].render_fence));

        VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                                   &frames[i].swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                                   &frames[i].render_semaphore));
    }
}

void VulkanEngine::init_descriptors() {
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

    global_descriptor_allocator.init_pool(device, 10, sizes);

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        draw_img_descriptor_layout =
            builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    draw_img_descriptors = global_descriptor_allocator.allocate(
        device, draw_img_descriptor_layout);

    VkDescriptorImageInfo img_info{};
    img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_info.imageView = draw_img.img_view;

    VkWriteDescriptorSet draw_img_write = {};
    draw_img_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    draw_img_write.pNext = nullptr;

    draw_img_write.dstBinding = 0;
    draw_img_write.descriptorCount = 1;
    draw_img_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    draw_img_write.pImageInfo = &img_info;

    vkUpdateDescriptorSets(device, 1, &draw_img_write, 0, nullptr);
}

void VulkanEngine::init_pipelines() { init_background_pipelines(); }

void VulkanEngine::init_background_pipelines() {
    VkPipelineLayoutCreateInfo compute_layout = {};
    compute_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compute_layout.pNext = nullptr;
    compute_layout.pSetLayouts = &draw_img_descriptor_layout;
    compute_layout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(device, &compute_layout, nullptr,
                                    &gradient_pipeline_layout));

    VkShaderModule compute_draw_shader;
    if (!vkutil::loader_shader_module("../shaders/gradient.comp.spv", device,
                                      &compute_draw_shader)) {
        fmt::print("Error when building the compute shader\n");
    }

    VkPipelineShaderStageCreateInfo stage_info = {};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.pNext = nullptr;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = compute_draw_shader;
    stage_info.pName = "main";

    VkComputePipelineCreateInfo compute_pipeline_create_info = {};
    compute_pipeline_create_info.sType =
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext = nullptr;
    compute_pipeline_create_info.layout = gradient_pipeline_layout;
    compute_pipeline_create_info.stage = stage_info;

    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                      &compute_pipeline_create_info, nullptr,
                                      &gradient_pipeline));

    vkDestroyShaderModule(device, compute_draw_shader, nullptr);

    main_deletion_queue.push_func([&]() {
            vkDestroyPipelineLayout(device, gradient_pipeline_layout, nullptr);
            vkDestroyPipeline(device, gradient_pipeline, nullptr);
    });
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchain_builder{active_gpu, device, surface};

    swapchain_img_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain =
        swapchain_builder
            .use_default_format_selection()
            // .set_desired_format(VkSurfaceFormatKHR{
            // .format = swapchain_img_format,
            // .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    swapchain_extent = vkb_swapchain.extent;
    swapchain = vkb_swapchain.swapchain;
    swapchain_imgs = vkb_swapchain.get_images().value();
    swapchain_img_views = vkb_swapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain() {
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    for (int i = 0; i < swapchain_img_views.size(); i++) {
        vkDestroyImageView(device, swapchain_img_views[i], nullptr);
    }
}

void VulkanEngine::cleanup() {
    if (is_init) {
        vkDeviceWaitIdle(device);
        main_deletion_queue.flush();

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(device, frames[i].command_pool, nullptr);

            vkDestroyFence(device, frames[i].render_fence, nullptr);
            vkDestroySemaphore(device, frames[i].render_semaphore, nullptr);
            vkDestroySemaphore(device, frames[i].swapchain_semaphore, nullptr);
        }

        destroy_swapchain();

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance, debug_messenger);
        vkDestroyInstance(instance, nullptr);

        SDL_DestroyWindow(window);
    }

    loaded_engine = nullptr;
}

void VulkanEngine::run() {
    SDL_Event e;
    bool quit = false;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }

            if (e.type == SDL_WINDOWEVENT) {
                switch (e.window.event) {
                case SDL_WINDOWEVENT_MINIMIZED:
                    stop_rendering = true;
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    stop_rendering = false;
                    break;
                }
            }
        }

        if (stop_rendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            draw();
        }
    }
}

void VulkanEngine::draw() {
    VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().render_fence, true,
                             1000000000));
    get_current_frame().deletion_queue.flush();
    VK_CHECK(vkResetFences(device, 1, &get_current_frame().render_fence));

    uint32_t swapchain_img_index;
    VkResult err = vkAcquireNextImageKHR(
        device, swapchain, 1000000000, get_current_frame().swapchain_semaphore,
        nullptr, &swapchain_img_index);
    if (err != VK_SUCCESS) {
        resize_requested = true;
        return;
    }
    fmt::println("hello{}", swapchain_img_index);

    VkCommandBuffer cmd = get_current_frame().main_command_buffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    draw_extent.width = draw_img.img_extent.width;
    draw_extent.height = draw_img.img_extent.height;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    vkutil::transition_img(cmd, draw_img.img, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    vkutil::transition_img(cmd, draw_img.img, VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_img(cmd, swapchain_imgs[swapchain_img_index],
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkutil::copy_img_to_img(cmd, draw_img.img,
                            swapchain_imgs[swapchain_img_index], draw_extent,
                            swapchain_extent);

    vkutil::transition_img(cmd, swapchain_imgs[swapchain_img_index],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info =
        vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        get_current_frame().swapchain_semaphore);
    VkSemaphoreSubmitInfo signal_info =
        vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                      get_current_frame().render_semaphore);

    VkSubmitInfo2 submit =
        vkinit::submit_info(&cmd_info, &signal_info, &wait_info);

    VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit,
                            get_current_frame().render_fence));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &swapchain;
    present_info.swapchainCount = 1;

    present_info.pWaitSemaphores = &get_current_frame().render_semaphore;
    present_info.waitSemaphoreCount = 1;

    present_info.pImageIndices = &swapchain_img_index;

    VkResult err_p = vkQueuePresentKHR(graphics_queue, &present_info);
    if (err_p != VK_SUCCESS) {
        resize_requested = true;
    }

    frame_num++;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd) {
    VkClearColorValue clear_val;
    float flash = abs(sin(frame_num / 120.0f));
    clear_val = {{0.0f, 0.0f, flash, 1.0f}};

    VkImageSubresourceRange clear_range =
        vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradient_pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradient_pipeline_layout, 0, 1, &draw_img_descriptors, 0, nullptr);
    //vkCmdClearColorImage(cmd, draw_img.img, VK_IMAGE_LAYOUT_GENERAL, &clear_val,
    //                    1, &clear_range);
}
