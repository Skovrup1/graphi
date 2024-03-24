#include "vk_engine.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "vk_descriptors.h"
#include "vk_init.h"
#include "vk_pipelines.h"
#include "vk_types.h"
#include "vk_util.h"

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

    init_imgui();

    is_init = true;
}

void VulkanEngine::init_imgui() {
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imgui_pool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_pool));

    // init imgui library, does not work with imgui versions after v1.89.9
    ImGui::CreateContext();

    ImGui_ImplSDL2_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = active_gpu;
    init_info.Device = device;
    init_info.Queue = graphics_queue;
    init_info.DescriptorPool = imgui_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;
    init_info.ColorAttachmentFormat = swapchain_img_format;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

    immediate_submit(
        [&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    main_deletion_queue.push_func([=, this]() {
        vkDestroyDescriptorPool(device, imgui_pool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
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

    // immediate submit
    VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr,
                                 &imm_command_pool));

    VkCommandBufferAllocateInfo cmd_alloc_info =
        vkinit::command_buffer_allocate_info(imm_command_pool, 1);

    VK_CHECK(
        vkAllocateCommandBuffers(device, &cmd_alloc_info, &imm_command_buffer));

    main_deletion_queue.push_func([=, this]() {
        vkDestroyCommandPool(device, imm_command_pool, nullptr);
    });
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

    // immediate submit
    VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &imm_fence));
    main_deletion_queue.push_func(
        [=, this]() { vkDestroyFence(device, imm_fence, nullptr); });
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
    draw_img_write.dstSet = draw_img_descriptors;
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

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(ComputePushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    compute_layout.pPushConstantRanges = &push_constant;
    compute_layout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(device, &compute_layout, nullptr,
                                    &gradient_pipeline_layout));

    VkShaderModule gradient_shader;
    if (!vkutil::loader_shader_module("shaders/gradient.comp.spv", device,
                                      &gradient_shader)) {
        fmt::print("Error when building the gradient shader\n");
    }

    VkShaderModule sky_shader;
    if (!vkutil::loader_shader_module("shaders/sky.comp.spv", device,
                                      &sky_shader)) {
        fmt::print("Error when building the sky shader\n");
    }

    VkPipelineShaderStageCreateInfo stage_info = {};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.pNext = nullptr;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = gradient_shader;
    stage_info.pName = "main";

    VkComputePipelineCreateInfo compute_pipeline_create_info = {};
    compute_pipeline_create_info.sType =
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext = nullptr;
    compute_pipeline_create_info.layout = gradient_pipeline_layout;
    compute_pipeline_create_info.stage = stage_info;

    ComputeEffect gradient;
    gradient.layout = gradient_pipeline_layout;
    gradient.name = "gradient";
    gradient.data = {};

    // default colors
    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);

    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                      &compute_pipeline_create_info, nullptr,
                                      &gradient.pipeline));

    compute_pipeline_create_info.stage.module = sky_shader;

    ComputeEffect sky;
    sky.layout = gradient_pipeline_layout;
    sky.name = "sky";
    sky.data = {};

    // defaults
    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                      &compute_pipeline_create_info, nullptr,
                                      &sky.pipeline));

    background_effects.push_back(gradient);
    background_effects.push_back(sky);

    // cleanup
    vkDestroyShaderModule(device, gradient_shader, nullptr);
    vkDestroyShaderModule(device, sky_shader, nullptr);

    main_deletion_queue.push_func([&]() {
        vkDestroyPipelineLayout(device, gradient_pipeline_layout, nullptr);
        vkDestroyPipeline(device, sky.pipeline, nullptr);
        vkDestroyPipeline(device, gradient.pipeline, nullptr);
    });
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchain_builder{active_gpu, device, surface};

    swapchain_img_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain =
        swapchain_builder.use_default_format_selection()
            .set_desired_format(VkSurfaceFormatKHR{
                .format = swapchain_img_format,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
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

            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        if (stop_rendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue; // skip drawing
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        draw();
    }
}

void VulkanEngine::draw() {
    VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().render_fence,
                             true, // this timeouts every time
                             1000000000));

    get_current_frame().deletion_queue.flush();

    uint32_t swapchain_img_index;
    VkResult err = vkAcquireNextImageKHR(
        device, swapchain, 1000000000, get_current_frame().swapchain_semaphore,
        nullptr, &swapchain_img_index);
    if (err != VK_SUCCESS) {
        resize_requested = true;
        return;
    }

    // draw_extent.width = std::min(swapchain_extent.width,
    // draw_img.img_extent.width) * render_scale; draw_extent.height =
    // std::min(swapchain_extent.height, draw_img.img_extent.height) *
    // render_scale;

    VK_CHECK(vkResetFences(device, 1, &get_current_frame().render_fence));

    VK_CHECK(vkResetCommandBuffer(get_current_frame().main_command_buffer, 0));

    VkCommandBuffer cmd = get_current_frame().main_command_buffer;

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
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    draw_imgui(cmd, swapchain_img_views[swapchain_img_index]);

    vkutil::transition_img(cmd, swapchain_imgs[swapchain_img_index],
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
    if (err_p != VK_SUCCESS) { // likely the source of linux vs windows bugs
        resize_requested = true;
    }

    frame_num++;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd) {
    ComputeEffect &effect = background_effects[current_background_effect];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            gradient_pipeline_layout, 0, 1,
                            &draw_img_descriptors, 0, nullptr);

    vkCmdPushConstants(cmd, gradient_pipeline_layout,
                       VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(ComputePushConstants), &effect.data);

    vkCmdDispatch(cmd, std::ceil(draw_extent.width / 16.0),
                  std::ceil(draw_extent.height / 16.0), 1);
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd,
                              VkImageView target_image_view) {
    VkRenderingAttachmentInfo color_attachment = vkinit::attachment_info(
        target_image_view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo render_info =
        vkinit::rendering_info(swapchain_extent, &color_attachment, nullptr);

    vkCmdBeginRendering(cmd, &render_info);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VulkanEngine::immediate_submit(
    std::function<void(VkCommandBuffer cmd)> &&function) {
    VK_CHECK(vkResetFences(device, 1, &imm_fence));
    VK_CHECK(vkResetCommandBuffer(imm_command_buffer, 0));

    VkCommandBuffer cmd = imm_command_buffer;

    VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info =
        vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit, imm_fence));

    VK_CHECK(vkWaitForFences(device, 1, &imm_fence, true, 9999999999));
}
