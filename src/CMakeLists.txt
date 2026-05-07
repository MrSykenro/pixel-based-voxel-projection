add_executable(main main.cpp)

target_include_directories(main PUBLIC ../thirdparty/glfw/include)

find_package(OpenGL)

target_link_libraries(main PRIVATE 
    test_lib
    voxel_projection
    OpenGL::GL
    glad
    glfw
    glm
)