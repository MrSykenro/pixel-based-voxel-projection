cmake_minimum_required(VERSION 4.0)
project(voxel-preojection LANGUAGES C CXX HIP)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(hip REQUIRED)

# Disable GLFW docs and examples
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glm)
add_subdirectory(lib)
add_subdirectory(src)