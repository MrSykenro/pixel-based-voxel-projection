# Create a fake hip::host library for hip-cpu if hip isnt detected.
if(NOT TARGET hip::host)
    find_package(Threads REQUIRED)
    find_package(TBB REQUIRED)
    
    add_library(hip::host INTERFACE IMPORTED)
    target_link_libraries(hip::host INTERFACE hip_cpu_rt::hip_cpu_rt)
endif()