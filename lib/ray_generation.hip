#include "voxel_projection.hpp"

__constant__ Matrix3x3f c_camera_matrices[32];
__constant__ float3 c_camera_positions[32];
__constant__ float c_focal_lengths[32];

// Calculate focal length using fov in degrees
static float calculateFocalLength(float fov_degrees){
    return 1.0f / tanf(fov_degrees * (M_PI / 360.0f));
}

/**
 * Calculate the final rotation matrix for rotating vectors in 3D space.
 * The rotation matrix is calculated to mimic Unity rotation.
 * It is left handed, Y-up and the multiplication order is M = Ry * Rx * Rz.
 * @param pitch X axis rotation in degrees
 * @param yaw   Y axis rotation in degrees
 * @param roll  Z axis rotation in degrees
 * @return a float array with 9 elements wrapped as a 3x3 matrix struct
 */
static Matrix3x3f calculateRotationMatrix(float pitch, float yaw, float roll){
    Matrix3x3f result;
    
    // Convert degrees into radians
    float p = pitch * (M_PI / 180.0f);
    float y = yaw   * (M_PI / 180.0f);
    float r = roll  * (M_PI / 180.0f);
    
    // Calculate sine and cosine of angles
    float sp = sinf(p), cp = cosf(p);
    float sy = sinf(y), cy = cosf(y);
    float sr = sinf(r), cr = cosf(r);
    
    // Calculate final rotation matrix
    result.matrix[0] = cy * cr + sy * sp * sr;
    result.matrix[1] = sr * cp;
    result.matrix[2] = -sy * cr + cy * sp * sr;
    
    result.matrix[3] = -cy * sr + sy * sp * cr;
    result.matrix[4] = cr * cp;
    result.matrix[5] = sy * sr + cy * sp * cr;
    
    result.matrix[6] = sy * cp;
    result.matrix[7] = -sp;
    result.matrix[8] = cy * cp;
    
    return result;
}

/**
 * Calculates the final direction of the ray.
 * Rotates the ray based on the cameras rotation matrix to match the world.
 * Then normalizes it to be used as a direction.
 * @param x, y, z Values of the ray.
 * @param m Corresponding rotation matrix of the camera.
 * @returns The final world direction of the ray
 */
__device__ __forceinline__ float3 createRay(float x, float y, float z, const Matrix3x3f& m){
    // Rotate the vector to match the world rotation
    float rotated_x = m.matrix[0] * x + m.matrix[1] * y + m.matrix[2] * z;
    float rotated_y = m.matrix[3] * x + m.matrix[4] * y + m.matrix[5] * z;
    float rotated_z = m.matrix[6] * x + m.matrix[7] * y + m.matrix[8] * z;

    // Normalize the vector to use as direction
    float invLen = rsqrtf(rotated_x * rotated_x + rotated_y * rotated_y + rotated_z * rotated_z);
    return make_float3(rotated_x * invLen, rotated_y * invLen, rotated_z * invLen);
}

/**
 * Generates all the rays using camera constants.
 * Writes all the data into corresponding arrays in the given RayBuffer.
 * Any ray after the RayBuffer fills is dismissed
 * @param difference_image Float array of the difference between 2 sets of frames.
 * @param ray_buffer Buffer to store the rays that are calculated.
 * @param threshold The minimum intensity to generate the ray.
 * @param image_width, image_height Image dimensions of a single frame.
 * 
 * TODO: Add a limit to the maximum number of rays.
 */
__global__ void calculateRayKernel(
    float* difference_image, 
    RayBuffer ray_buffer, 
    float threshold,
    int image_width, int image_height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int camera_id = blockIdx.z;
    
    if(x >= image_width || y >= image_height) return;

    // Get the index of the current pixel.
    // (iz * height * width) + (iy * width) + ix    
    // =>   ([iz * height] + iy) * width + ix 
    size_t pixel_index = (size_t)(camera_id * image_height + y) * (size_t)image_width + x;

    // Prevent ray overflow
    if(pixel_index >= ray_buffer.max_ray_count) return;

    // Prevent unnecessary ray calculation.
    float intensity = difference_image[pixel_index];
    if(intensity < threshold) return;
    

    // Create the ray
    float3 ray = createRay(
        (float)x - (image_width * 0.5f),
        (float)y - (image_height * 0.5f),
        c_focal_lengths[camera_id],
        c_camera_matrices[camera_id]
    );

    // Increase the calculated ray count and read the index of the current ray.
    int ray_index = atomicAdd(ray_buffer.ray_count, 1);

    // Write the current ray into it's corresponding spot. 
    ray_buffer.camera_id[ray_index] = camera_id;
    ray_buffer.dir_x[ray_index] = ray.x;
    ray_buffer.dir_y[ray_index] = ray.y;
    ray_buffer.dir_z[ray_index] = ray.z;
    ray_buffer.intensity[ray_index] = intensity;
}


extern "C"{namespace VoxelP{
    RayBuffer initRayBuffer(size_t max_ray_count){
        float *d_dir_x, *d_dir_y, *d_dir_z, *d_intensity;
        int *d_camera_id, *d_ray_count;

        // Allocate memory on the gpu.
        HIP_CHECK(hipMalloc(&d_dir_x, max_ray_count * sizeof(float)));
        HIP_CHECK(hipMalloc(&d_dir_y, max_ray_count * sizeof(float)));
        HIP_CHECK(hipMalloc(&d_dir_z, max_ray_count * sizeof(float)));
        HIP_CHECK(hipMalloc(&d_intensity, max_ray_count * sizeof(float)));
        HIP_CHECK(hipMalloc(&d_camera_id, max_ray_count * sizeof(int)));
        HIP_CHECK(hipMalloc(&d_ray_count, sizeof(int)));

        HIP_CHECK(hipMemset(&d_ray_count, 0, sizeof(int)));

        return RayBuffer{
            d_dir_x, d_dir_y, d_dir_z, d_intensity, d_camera_id , d_ray_count, max_ray_count
        };
    }
    
    void freeRayBuffer(RayBuffer &ray_buffer){
        // Free memory on the gpu.
        HIP_CHECK(hipFree(ray_buffer.dir_x));
        HIP_CHECK(hipFree(ray_buffer.dir_y));
        HIP_CHECK(hipFree(ray_buffer.dir_z));
        HIP_CHECK(hipFree(ray_buffer.intensity));
        HIP_CHECK(hipFree(ray_buffer.camera_id));
        HIP_CHECK(hipFree(ray_buffer.ray_count));

        ray_buffer.dir_x = nullptr;
        ray_buffer.dir_y = nullptr;
        ray_buffer.dir_z = nullptr;
        ray_buffer.intensity = nullptr;
        ray_buffer.camera_id = nullptr;
        ray_buffer.ray_count = nullptr;

        ray_buffer.max_ray_count = 0;
    }

    void setCameraConstants(const CameraData* camera_data_array, int camera_count){
        if(camera_count > 32){
            std::cerr << "[Memory Limit Error]: camera_count(" << camera_count 
            <<")exceeds the limit of 32. Only the first 32 cameras will be included" << std::endl;
            camera_count = 32;
        }

        Matrix3x3f  h_camera_matrices[32];
        float3      h_camera_positions[32];
        float       h_focal_lengths[32];

        for(int i = 0; i < camera_count; i++){
            const CameraData &camera = camera_data_array[i];

            h_camera_matrices[i] = calculateRotationMatrix(camera.rot_x, camera.rot_y, camera.rot_z);
            h_camera_positions[i] = make_float3(camera.pos_x, camera.pos_y, camera.pos_z);
            h_focal_lengths[i] = calculateFocalLength(camera.fov_degrees);
        }

        // Copy camera data to device's constant memory
        hipMemcpyToSymbol(HIP_SYMBOL(c_camera_matrices),h_camera_matrices, sizeof(Matrix3x3f) * camera_count);
        hipMemcpyToSymbol(HIP_SYMBOL(c_camera_positions), h_camera_positions, sizeof(float3) * camera_count);
        hipMemcpyToSymbol(HIP_SYMBOL(c_focal_lengths), h_focal_lengths, sizeof(float) * camera_count);
    }

    void generateRays(
        float* difference_image,
        RayBuffer ray_buffer,
        float threshold,
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
            calculateRayKernel, grid_size, block_size, 0, 0, 
            difference_image, ray_buffer, threshold, image_width, image_height
        );
    }
}}