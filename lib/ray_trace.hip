#include <voxel_projection.hpp>

#include <thrust/device_ptr.h>
#include <thrust/scan.h>
#include <thrust/reduce.h>
#include <cmath>

#define EPSILON 1e-12f
extern __constant__ float3 c_camera_positions[32];

/**
 * Calculates the length of the ray when it enters and its the grid,
 * to be used by the DDA algorithm
 * @param ray_origin Starting position of the ray in world coordinates
 * @param ray_direction Direction of the ray.
 * @param inverse_direction Inverse of the ray_direction.
 * @param grid_min Starting position of the grid based on the world coordinates.
 * @param grid_max Ending position of the grid based on the world coordinates.
 * @param ray_min, ray_max The address for the calculation results to be written onto.
 */
__device__ __forceinline__ bool calculateIntersectionPoints(
    const float3 ray_origin, const float3 ray_direction, float3 inverse_direction, 
    const float3 grid_min, const float3 grid_max,
    float& ray_min, float& ray_max)
{
    // Check if the parallel ray intersects the grid. Return false if not.
    
    ray_min = 0.f; 
    ray_max = FLT_MAX;
    float ray_A, ray_B;

    // X axis
    if(fabsf(ray_direction.x) < EPSILON) {if(ray_origin.x < grid_min.x || ray_origin.x > grid_max.x) return false;}
    else{
        ray_A = (grid_min.x - ray_origin.x) * inverse_direction.x;
        ray_B = (grid_max.x - ray_origin.x) * inverse_direction.x;
        ray_min = fmaxf(ray_min, fminf(ray_A, ray_B));
        ray_max = fminf(ray_max, fmaxf(ray_A, ray_B));
    }
    // Y axis
    if(fabsf(ray_direction.y) < EPSILON) {if(ray_origin.y < grid_min.y || ray_origin.y > grid_max.y) return false;}
    else{
        ray_A = (grid_min.y - ray_origin.y) * inverse_direction.y;
        ray_B = (grid_max.y - ray_origin.y) * inverse_direction.y;
        ray_min = fmaxf(ray_min, fminf(ray_A, ray_B));
        ray_max = fminf(ray_max, fmaxf(ray_A, ray_B));
    }
    // Z axis
    if(fabsf(ray_direction.z) < EPSILON) {if(ray_origin.z < grid_min.z || ray_origin.z > grid_max.z) return false;}
    else{
        ray_A = (grid_min.z - ray_origin.z) * inverse_direction.z;
        ray_B = (grid_max.z - ray_origin.z) * inverse_direction.z;
        ray_min = fmaxf(ray_min, fminf(ray_A, ray_B));
        ray_max = fminf(ray_max, fmaxf(ray_A, ray_B));
    }
    
    if(ray_min < 0.f) ray_min = 0.f;
    if(ray_min > ray_max) return false;
    return true;
}

/**
 * Mark the chunks on the given chukn grid that are traversed by the given rays.
 * @param ray_buffer All of the rays that will be checked
 * @param voxel_grid The grid that will be checked
 */
__global__ void findActiveChunks(RayBuffer ray_buffer, VoxelGrid voxel_grid){
    int ray_index = blockIdx.x * blockDim.x + threadIdx.x;

    if(ray_index >= *(ray_buffer.ray_count)) return;

    float chunk_length = voxel_grid.chunk_size * voxel_grid.voxel_length;
    // Create and calculate the values that will be used
    int camera_id = ray_buffer.camera_id[ray_index];
    float3 ray_origin = c_camera_positions[camera_id];
    float3 ray_direction = make_float3(
        ray_buffer.dir_x[ray_index],
        ray_buffer.dir_y[ray_index],
        ray_buffer.dir_z[ray_index]
    );
    float3 inverse_direction = make_float3(
        1.f / ray_direction.x,
        1.f / ray_direction.y,
        1.f / ray_direction.z
    );

    // Calculate the start and end coordinates of the grid
    float3 grid_min = make_float3(
        voxel_grid.grid_position_x, 
        voxel_grid.grid_position_y, 
        voxel_grid.grid_position_z
    );
    float3 grid_max = make_float3(
        voxel_grid.grid_position_x + voxel_grid.chunk_grid_x * chunk_length, 
        voxel_grid.grid_position_y + voxel_grid.chunk_grid_y * chunk_length, 
        voxel_grid.grid_position_z + voxel_grid.chunk_grid_z * chunk_length
    );

    // Calculate length of the ray when it enters and exits the grid.
    float ray_min, ray_max;
    bool is_collision = calculateIntersectionPoints(
        ray_origin, ray_direction, inverse_direction, 
        grid_min, grid_max, ray_min, ray_max
    );
    if(!is_collision) return;

    // Calculate cooridnates of the point where the ray first enters the grid.
    float3 start_world = ray_origin + ray_min * ray_direction;

    // Calculate the index of the first voxel.
    // TODO: Check for edge case errors with grid_max
    int3 chunk_index = make_int3(
        static_cast<int>((start_world.x - grid_min.x) / chunk_length),
        static_cast<int>((start_world.y - grid_min.y) / chunk_length),
        static_cast<int>((start_world.z - grid_min.z) / chunk_length)
    );

    // Check if the ray is outside.
    if( chunk_index.x < 0 || chunk_index.x >= voxel_grid.chunk_grid_x || 
        chunk_index.y < 0 || chunk_index.y >= voxel_grid.chunk_grid_y ||
        chunk_index.z < 0 || chunk_index.z >= voxel_grid.chunk_grid_z
    ) return;

    int3 step_direction = make_int3(
        (ray_direction.x >= 0) ? 1 : -1,
        (ray_direction.y >= 0) ? 1 : -1,
        (ray_direction.z >= 0) ? 1 : -1
    );

    // Calculate the begining world coordinates of the indexed voxel.
    float3 entrance_plane = make_float3(
        static_cast<float>(chunk_index.x + max(0, step_direction.x)) * chunk_length + grid_min.x,
        static_cast<float>(chunk_index.y + max(0, step_direction.y)) * chunk_length + grid_min.y,
        static_cast<float>(chunk_index.z + max(0, step_direction.z)) * chunk_length + grid_min.z
    );

    // Length of the ray; from origin to nearest voxel boundary plane.  
    float3 ray_side = (entrance_plane - ray_origin) * inverse_direction;
    // Length of the ray that needs to be traveled per iteration.  
    float3 ray_delta = make_float3(
        chunk_length * fabsf(inverse_direction.x),
        chunk_length * fabsf(inverse_direction.y),
        chunk_length * fabsf(inverse_direction.z)
    );

    // Slowly increase the size of the ray to find all the chunks activated by it.
    float ray_current = ray_min;
    while(ray_current <= ray_max){
        size_t index = 
            (size_t)voxel_grid.chunk_grid_x * (size_t)voxel_grid.chunk_grid_y * (size_t)chunk_index.z +
            (size_t)voxel_grid.chunk_grid_x * (size_t)chunk_index.y +
            (size_t)chunk_index.x
        ;

        // Mark the chunks as active
        if(voxel_grid.chunk_mask[index] == 0) atomicExch(&voxel_grid.chunk_mask[index], 1);
        
        // Iteration Logic
        if(ray_side.x < ray_side.y && ray_side.x < ray_side.z){
            chunk_index.x += step_direction.x;
            ray_current = ray_side.x;
            ray_side.x += ray_delta.x;
        }
        else if(ray_side.y < ray_side.z){
            chunk_index.y += step_direction.y;
            ray_current = ray_side.y;
            ray_side.y += ray_delta.y;
        }
        else{
            chunk_index.z += step_direction.z;
            ray_current = ray_side.z;
            ray_side.z += ray_delta.z;
        }
        
        // Owerflow prevention
        if(
            chunk_index.x < 0 || chunk_index.x >= voxel_grid.chunk_grid_x ||
            chunk_index.y < 0 || chunk_index.y >= voxel_grid.chunk_grid_y ||
            chunk_index.z < 0 || chunk_index.z >= voxel_grid.chunk_grid_z
        ) break;
    }
}

__global__ void traceRays(RayBuffer ray_buffer, VoxelGrid voxel_grid){
    int ray_index = blockIdx.x * blockDim.x + threadIdx.x;
    
    if(ray_index >= *(ray_buffer.ray_count)) return;

    int3 grid_size = make_int3(
        voxel_grid.chunk_grid_x * voxel_grid.chunk_size,
        voxel_grid.chunk_grid_y * voxel_grid.chunk_size,
        voxel_grid.chunk_grid_z * voxel_grid.chunk_size
    );
    // Create and calculate the values that will be used
    const int bit_mask = (1 << voxel_grid.chunk_size_ln2) - 1; 
    int camera_id = ray_buffer.camera_id[ray_index];
    float3 ray_origin = c_camera_positions[camera_id];
    float3 ray_direction = make_float3(
        ray_buffer.dir_x[ray_index],
        ray_buffer.dir_y[ray_index],
        ray_buffer.dir_z[ray_index]
    );
    float3 inverse_direction = make_float3(
        1.f / ray_direction.x,
        1.f / ray_direction.y,
        1.f / ray_direction.z
    );

    // Calculate the start and end coordinates of the grid
    float3 grid_min = make_float3(
        voxel_grid.grid_position_x, 
        voxel_grid.grid_position_y, 
        voxel_grid.grid_position_z
    );
    float3 grid_max = make_float3(
        voxel_grid.grid_position_x + grid_size.x * voxel_grid.voxel_length, 
        voxel_grid.grid_position_y + grid_size.y * voxel_grid.voxel_length, 
        voxel_grid.grid_position_z + grid_size.z * voxel_grid.voxel_length
    );

    // Calculate length of the ray when it enters and exits the grid.
    float ray_min, ray_max;
    bool is_collision = calculateIntersectionPoints(
        ray_origin, ray_direction, inverse_direction, 
        grid_min, grid_max, ray_min, ray_max
    );
    if(!is_collision) return;

    // Calculate cooridnates of the point where the ray first enters the grid.
    float3 start_world = ray_origin + ray_min * ray_direction;

    // Calculate the index of the first voxel.
    // TODO: Check for edge case errors with grid_max
    int3 voxel_index = make_int3(
        static_cast<int>((start_world.x - grid_min.x) / voxel_grid.voxel_length),
        static_cast<int>((start_world.y - grid_min.y) / voxel_grid.voxel_length),
        static_cast<int>((start_world.z - grid_min.z) / voxel_grid.voxel_length)
    );

    // Check if the ray is outside.
    if( voxel_index.x < 0 || voxel_index.x >= grid_size.x || 
        voxel_index.y < 0 || voxel_index.y >= grid_size.y ||
        voxel_index.z < 0 || voxel_index.z >= grid_size.z
    ) return;

    int3 step_direction = make_int3(
        (ray_direction.x >= 0) ? 1 : -1,
        (ray_direction.y >= 0) ? 1 : -1,
        (ray_direction.z >= 0) ? 1 : -1
    );

    // Calculate the begining world coordinates of the indexed voxel.
    float3 entrance_plane = make_float3(
        static_cast<float>(voxel_index.x + max(0, step_direction.x)) * voxel_grid.voxel_length + grid_min.x,
        static_cast<float>(voxel_index.y + max(0, step_direction.y)) * voxel_grid.voxel_length + grid_min.y,
        static_cast<float>(voxel_index.z + max(0, step_direction.z)) * voxel_grid.voxel_length + grid_min.z
    );

    // Length of the ray; from origin to nearest voxel boundary plane.  
    float3 ray_side = (entrance_plane - ray_origin) * inverse_direction;
    // Length of the ray that needs to be traveled per iteration.  
    float3 ray_delta = make_float3(
        voxel_grid.voxel_length * fabsf(inverse_direction.x),
        voxel_grid.voxel_length * fabsf(inverse_direction.y),
        voxel_grid.voxel_length * fabsf(inverse_direction.z)
    );
    
    float intensity = ray_buffer.intensity[ray_index];
    float ray_current = ray_min;

    while (ray_current <= ray_max) {
        // Calculate the position of the chunk
        int3 chunk_position = make_int3(
            voxel_index.x >> voxel_grid.chunk_size_ln2,
            voxel_index.y >> voxel_grid.chunk_size_ln2,
            voxel_index.z >> voxel_grid.chunk_size_ln2
        );
        // Calculate linear index of the chunk
        size_t chunk_index = 
            (size_t)voxel_grid.chunk_grid_x * (size_t)voxel_grid.chunk_grid_y * (size_t)chunk_position.z +
            (size_t)voxel_grid.chunk_grid_x * (size_t)chunk_position.y +
            (size_t)chunk_position.x;
        
        // Check if the chunk is active
        if (voxel_grid.active_chunks[chunk_index] != 0) {
            // Get the starting position of the chunk
            int chunk_begin = voxel_grid.chunk_mask[chunk_index];

            // Calculate local voxel position within the chunk
            int3 local_voxel_position = make_int3(
                voxel_index.x & bit_mask,
                voxel_index.y & bit_mask,
                voxel_index.z & bit_mask
            );

            // Calculate the index of the voxel 
            size_t local_index = 
                (local_voxel_position.z << (voxel_grid.chunk_size_ln2 * 2)) + 
                (local_voxel_position.y << voxel_grid.chunk_size_ln2) + 
                local_voxel_position.x
            ;
            size_t global_index = (size_t)chunk_begin * (1 << (voxel_grid.chunk_size_ln2 * 3)) + local_index;

            // Accumulate intensity
            atomicAdd(&voxel_grid.voxel_array[global_index], intensity);
        }

        // Iteration Logic
        if (ray_side.x < ray_side.y && ray_side.x < ray_side.z) {
            voxel_index.x += step_direction.x;
            ray_current = ray_side.x;
            ray_side.x += ray_delta.x;
        } else if (ray_side.y < ray_side.z) {
            voxel_index.y += step_direction.y;
            ray_current = ray_side.y;
            ray_side.y += ray_delta.y;
        } else {
            voxel_index.z += step_direction.z;
            ray_current = ray_side.z;
            ray_side.z += ray_delta.z;
        }

        // Owerflow prevention
        if (voxel_index.x < 0 || voxel_index.x >= grid_size.x ||
            voxel_index.y < 0 || voxel_index.y >= grid_size.y ||
            voxel_index.z < 0 || voxel_index.z >= grid_size.z
        ) break;
    }
}

extern "C"{namespace VoxelP{
    VoxelGrid initVoxelGrid(
        int voxel_length, int chunk_size,
        float grid_position_x, float grid_position_y, float grid_position_z,
        int chunk_grid_x, int chunk_grid_y, int chunk_grid_z  
    ){
        int chunk_size_ln2 = (int)log2((double)chunk_size); // Cause HIP and MSVC cant get along
        size_t chunk_count = chunk_grid_x * chunk_grid_y * chunk_grid_z;

        int* active_chunks;
        int* chunk_mask;
        float* voxel_array;
        
        HIP_CHECK(hipMalloc(&active_chunks, chunk_count * sizeof(int)));
        HIP_CHECK(hipMalloc(&chunk_mask, chunk_count * sizeof(int)));
        HIP_CHECK(hipMalloc(&voxel_array, sizeof(int)));

        HIP_CHECK(hipMemset(active_chunks, 0, chunk_count * sizeof(int)));
        HIP_CHECK(hipMemset(chunk_mask, 0, chunk_count * sizeof(int)));
        HIP_CHECK(hipMemset(voxel_array, 0, sizeof(int)));

        return VoxelGrid{
            voxel_length, chunk_size, chunk_size_ln2, 
            grid_position_x, grid_position_y, grid_position_z, 
            chunk_grid_x, chunk_grid_y, chunk_grid_z,
            active_chunks, chunk_mask, voxel_array
        };
    }

    void projectToGrid(RayBuffer ray_buffer, VoxelGrid &voxel_grid){
        int thread_count = 256;
        int block_count = (*ray_buffer.ray_count + thread_count - 1) / thread_count;

        size_t total_chunks = voxel_grid.chunk_grid_x * voxel_grid.chunk_grid_y * voxel_grid.chunk_grid_z;

        // Clear active_chunks array
        HIP_CHECK(hipMemset(voxel_grid.active_chunks, 0, total_chunks * sizeof(int)));

        hipLaunchKernelGGL(findActiveChunks, dim3(block_count), dim3(thread_count), 0, 0, ray_buffer, voxel_grid);
        hipDeviceSynchronize();

        // Convert active chunks array into a chunk mask
        thrust::device_ptr<int> d_active_chunks(voxel_grid.chunk_mask);
        thrust::device_ptr<int> d_chunk_mask(voxel_grid.chunk_mask);
        thrust::exclusive_scan(d_active_chunks, d_active_chunks + total_chunks, d_chunk_mask);

        int active_chunk_count = thrust::reduce(d_active_chunks, d_active_chunks + total_chunks, 0, thrust::plus<int>());
        if (active_chunk_count == 0) return;

        size_t total_voxels = (size_t)active_chunk_count * (size_t)1 << (voxel_grid.chunk_size_ln2 * 3);

        // Create new voxel_array
        HIP_CHECK(hipMalloc(&voxel_grid.voxel_array, total_voxels * sizeof(float)));
        HIP_CHECK(hipMemset(voxel_grid.voxel_array, 0, total_voxels * sizeof(float)));

        hipLaunchKernelGGL(traceRays, dim3(block_count), dim3(thread_count), 0, 0, ray_buffer, voxel_grid);
        hipDeviceSynchronize();
    }
}}