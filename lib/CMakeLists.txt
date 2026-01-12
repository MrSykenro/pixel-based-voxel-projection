# Create libraries
add_library(voxel_lib STATIC 
    test.hip
)

target_include_directories(voxel_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link HIP to libraries
target_link_libraries(voxel_lib PRIVATE hip::host)