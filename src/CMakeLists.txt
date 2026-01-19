add_executable(main main.cpp)

target_link_libraries(main PRIVATE 
    test_lib
    voxel_projection
)