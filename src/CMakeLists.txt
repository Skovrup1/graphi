cmake_minimum_required(VERSION 3.20)

project(graphi VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# to remove warnings in clang
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")

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
    vk_init.h
    vk_init.cpp
    vk_util.h
    vk_util.cpp
    vk_descriptors.h
    vk_descriptors.cpp
    vk_pipelines.h
    vk_pipelines.cpp
    stb_image.h
    tiny_obj_loader.h
    )

target_include_directories(main PUBLIC 
    sdl
    imgui
    imgui/backends
    vma/include
    vk-bootstrap/src
    )


# IMGUI
add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE
    imgui/imgui.h
    imgui/imgui.cpp

    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp

    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    )

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL2::SDL2)

target_link_libraries(
    main
    SDL2::SDL2
    fmt::fmt
    Vulkan::Vulkan
    glm::glm
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
    imgui
    )

IF (NOT WIN32)
    target_link_libraries(
        main
        m
        )
ENDIF()

include(CMakePrintHelpers)

find_program(GLSL_VALIDATOR glslangValidator HINTS
    /usr/bin
    /usr/local/bin
    $ENV{VULKAN_SDK}/Bin/
    $ENV{VULKAN_SDK}/Bin32/
    REQUIRED)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/../shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/../shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/../shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/../shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
    )

