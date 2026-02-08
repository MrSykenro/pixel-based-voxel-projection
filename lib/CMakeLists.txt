# TODO: Use a foreach loop instead of setting all the libraries manually

# Create libraries
add_library(test_lib STATIC test.hip)
add_library(voxel_projection STATIC 
    ray_generation.hip
    motion_extraction.hip
)

target_include_directories(test_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(voxel_projection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link HIP to libraries
target_link_libraries(test_lib PRIVATE hip::host)
target_link_libraries(voxel_projection PRIVATE hip::host)