#include "voxel_projection.hpp"

// Conversion constants for grayscale (Rec.601)
#define GS_R 0.299f
#define GS_G 0.587f
#define GS_B 0.114f

/**
 * Calculates the differences between given frames by subtracting them.
 * Uses Rec.601 standard to convert the pixel value into grayscale.
 * Stores all the output frames from different cameras into a single float array.
 * @param frame_A Input frame A.
 * @param frame_B Input frame B.
 * @param motion_output Output array address
 * @param image_width, image_height Image dimensions of a single frame.
 */
__global__ void extractMotionKernel(
    const uchar4* __restrict__ frame_A, 
    const uchar4* __restrict__ frame_B,
    float* __restrict__ motion_output,
    int image_width, int image_height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int camera_id = blockIdx.z;

    if(x >= image_width || y >= image_height) return;

    // Calculate the position of the current pixel in the global array.
    // (iz * height * width) + (iy * width) + ix    
    // =>   ([iz * height] + iy) * width + ix 
    size_t pixel_index = (size_t)(camera_id * image_height + y) * (size_t)image_width + x;

    uchar4 pixel_A = frame_A[pixel_index];
    uchar4 pixel_B = frame_A[pixel_index];

    // Calculate grayscale values of each frame and take their difference
    float grayscale_A = pixel_A.x * GS_R + pixel_A.y * GS_G + pixel_A.z * GS_B;
    float grayscale_B = pixel_B.x * GS_R + pixel_B.y * GS_G + pixel_B.z * GS_B;
    float difference = fabsf(grayscale_A - grayscale_B);

    // Write the difference to the global memory
    motion_output[pixel_index] = difference;
} 


extern "C"{namespace VoxelP{
    void extractMotion(
        const uchar4* d_frame_A,
        const uchar4* d_frame_B,
        float* d_motion_output,
        int image_width, int image_height,
        int camera_count)
    {
        dim3 block_size(16, 16, 1);

        dim3 grid_size(
            (image_width + block_size.x - 1) / block_size.x,
            (image_height + block_size.y - 1) / block_size.y,
            camera_count
        );

        hipLaunchKernelGGL(
            extractMotionKernel, grid_size, block_size, 0, 0, 
            d_frame_A, d_frame_B, d_motion_output, image_width, image_height
        );
    }
}}
