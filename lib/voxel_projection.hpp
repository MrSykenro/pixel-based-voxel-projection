#pragma once
#define _USE_MATH_DEFINES

#include "formats.hpp"
#include <iostream>
#include <math.h>
#include <hip/hip_runtime.h>
#include <hip/hip_gl_interop.h>

#define HIP_CHECK(command) { \
    hipError_t status = command; \
    if (status != hipSuccess) { \
        std::cerr << "HIP Error: " << hipGetErrorString(status) << std::endl; \
        exit(1); \
    } \
}

extern __constant__ Matrix3x3f c_camera_matrices[32];
extern __constant__ float3 c_camera_positions[32];
extern __constant__ float c_focal_lengths[32];

extern "C"{namespace VoxelP{ 
    /**
     * Create a RayBuffer struct with all the arrays allocated in the gpu.
     * @param max_ray_count Maximum amount of rays that can be stored.
     * 
     * TODO: Consider using C++ classes instead.
     */
    RayBuffer initRayBuffer(size_t max_ray_count);
    
    /**
     * Free the specified RayBuffer.
     * @param ray_buffer Specify the RayBuffer to free.
     * 
     * TODO: Consider using C++ classes instead.
     */
    void freeRayBuffer(RayBuffer &ray_buffer);

    /**
     * Calculates the motion differences between stacked frames by subtracting them.
     * Input frames must be stored as a single array contaning all the images from different cameras.
     * @param d_frame_A Device pointer of input frame A.
     * @param d_frame_B Device pointer of input frame B.
     * @param d_motion_output Device pointer of output array address
     * @param image_width, image_height Image dimensions of a single frame.
     * 
     * @warning Both input arrays must stack the frames in the same order.
     */
    void extractMotion(
        const uchar4* d_frame_A, const uchar4* d_frame_B, float* d_motion_output,
        int image_width, int image_height, int camera_count
    );

    /**
     * Sets the camera data into constant memory.
     * The array is limited by 32 elements (cameras).
     * @param camera_data_array The CameraData struct array containing information of all the cameras
     * @param camera_count The number of that cameras are included in the array. (Limited by 32)
     */
    void setCameraConstants(const CameraData* camera_data_array, int camera_count);

    /**
     * Generates rays using the camera constants and the given difference image.
     * The output is a struct of arrays each one containing a different section of the rays.
     * @param difference_image Float array of the difference between 2 sets of frames.
     * @param ray_buffer Buffer to store the rays that are calculated.
     * @param threshold The minimum intensity to generate the ray.
     * @param image_width, image_height Image dimensions of a single frame.
     * 
     * @warning Camera constants must be set before hand by calling setCameraConstants
     */
    void generateRays(
        float* difference_image, RayBuffer ray_buffer, float threshold, 
        int image_width, int image_height, int camera_count
    );

    //--------------------------------------------------------------------------
    // Requires Documentation
    //--------------------------------------------------------------------------
    void projectToGrid(RayBuffer ray_buffer, VoxelGrid &voxel_grid);

    VoxelGrid initVoxelGrid(
        int voxel_length, int chunk_size,
        float grid_position_x, float grid_position_y, float grid_position_z,
        int chunk_grid_x, int chunk_grid_y, int chunk_grid_z  
    );

    void freeVoxelGrid(VoxelGrid &voxel_grid);

    /**
     * Converts voxel_grid into coordinate format to be used for Rendering
     */
    void convertToVBO(VoxelGrid &voxel_grid, float* opengl_VBO);

}}
