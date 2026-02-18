#include "voxel_projection.hpp"

#define EPSILON 1e-12f

/**
 * Calculates the length of the ray when it enters and exits the grid,
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

    // Create and calculate the values that will be used
    float chunk_length = voxel_grid.chunk_size * voxel_grid.voxel_length;
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
        voxel_grid.grid_position_z + voxel_grid.chunk_grid_z * chunk_length);

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
    if( chunk_index.x<0 || chunk_index.x >= voxel_grid.chunk_grid_x || 
        chunk_index.y<0 || chunk_index.y >= voxel_grid.chunk_grid_y ||
        chunk_index.z<0 || chunk_index.z >= voxel_grid.chunk_grid_z)
    return;

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
