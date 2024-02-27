cmake_minimum_required(VERSION 3.5)

project(graphi VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

#imgui
include_directories(imgui imgui/backends)

#vk bootstrap
add_subdirectory(vk-bootstrap)
#sdl2
add_subdirectory(sdl)
#fmt
add_subdirectory(fmt)
#glm
add_subdirectory(glm)

#find vulkan on windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.275.0")
    set(Vulkan_LIBRARY "C:/VulkanSDK/1.3.275.0/Lib")
endif()
find_package(Vulkan REQUIRED)

add_executable(main main.cpp vk_engine.cpp vk_engine.h vk_types.h vk_init.cpp vk_init.h vk_imgs.cpp vk_imgs.h)

target_link_libraries(main glm::glm)
target_link_libraries(main vk-bootstrap::vk-bootstrap)
target_link_libraries(main SDL2::SDL2 fmt::fmt Vulkan::Vulkan)

