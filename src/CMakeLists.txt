cmake_minimum_required(VERSION 3.5)

project(graphi VERSION 0.0.0 LANGUAGES CXX C)

add_library(Foo SHARED lib.cpp)

add_executable(graphi_app main.cpp)

target_link_libraries(graphi_app PRIVATE Foo)
