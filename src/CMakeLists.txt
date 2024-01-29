cmake_minimum_required(VERSION 3.5)

project(graphi VERSION 0.0.0 LANGUAGES CXX C)

add_executable(graphi_app main.cpp)

add_subdirectory(vk-bootstrap)

find_package(glm CONFIG REQUIRED)

target_link_libraries(graphi_app PRIVATE glm::glm-header-only)

target_link_libraries(graphi_app PRIVATE vk-bootstrap::vk-bootstrap)


