cmake_minimum_required(VERSION 3.5)

project(graphi VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

#imgui
include_directories(imgui imgui/backends)

#vk bootstrap
add_subdirectory(vk-bootstrap)

#glm
find_package(glm CONFIG REQUIRED)

add_executable(graphi_app main.cpp)

target_link_libraries(graphi_app PRIVATE glm::glm-header-only)
target_link_libraries(graphi_app PRIVATE vk-bootstrap::vk-bootstrap)


