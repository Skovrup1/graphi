cmake_minimum_required(VERSION 3.5)

project(graphi VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

#imgui
include_directories(imgui imgui/backends)

#vk bootstrap
add_subdirectory(vk-bootstrap)

#find packages
find_package(glm CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(fmt REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(graphi_app main.cpp vk_engine.cpp vk_engine.h vk_types.h)

target_link_libraries(graphi_app glm::glm-header-only)
target_link_libraries(graphi_app vk-bootstrap::vk-bootstrap)
target_link_libraries(graphi_app SDL2 fmt Vulkan::Vulkan)

