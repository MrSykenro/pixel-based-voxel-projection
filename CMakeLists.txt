cmake_minimum_required(VERSION 4.0)
project(voxel-preojection LANGUAGES CXX HIP)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(hip REQUIRED)

add_subdirectory(lib)
add_subdirectory(src)