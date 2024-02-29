cmake_minimum_required(VERSION 3.20)

project(graphi VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")

include_directories(imgui imgui/backends)
add_subdirectory(vk-bootstrap)
add_subdirectory(sdl)
add_subdirectory(fmt)
add_subdirectory(glm)
add_subdirectory(vma)

find_package(Vulkan REQUIRED)

add_executable(
    main
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_init.cpp
    vk_init.h
    vk_util.cpp
    vk_util.h
    stb_image.h
    tiny_obj_loader.h
)

target_link_libraries(
    main
    SDL2::SDL2
    fmt::fmt
    Vulkan::Vulkan
    glm::glm
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
)

