#include <voxel_projection.hpp>

__global__ void convertVoxelToVBOKernel(VoxelGrid voxel_grid, float* opengl_VBO){
    // Get chunk and local voxel index in voxel_grid
    int local_index = 
        (threadIdx.z << (voxel_grid.chunk_size_ln2 * 2)) + 
        (threadIdx.y << voxel_grid.chunk_size_ln2) + 
        threadIdx.x
    ;
    int chunk_index = voxel_grid.chunk_array[blockIdx.x];
    
    // Calculate write index
    size_t global_index = (blockIdx.x << (voxel_grid.chunk_size_ln2*3)) + local_index;

    // Calculate chunk coordinates
    int chunk_position_z = chunk_index / (voxel_grid.chunk_grid_x*voxel_grid.chunk_grid_y);
    int chunk_position_y = (chunk_index - (voxel_grid.chunk_grid_x*voxel_grid.chunk_grid_y*chunk_position_z)) / voxel_grid.chunk_grid_x;
    int chunk_position_x = (chunk_index - (voxel_grid.chunk_grid_x*voxel_grid.chunk_grid_y*chunk_position_z) - voxel_grid.chunk_grid_x*chunk_position_y);

    // Calculate world coordinates
    float coordinate_x = (voxel_grid.chunk_size*chunk_position_x * voxel_grid.voxel_length) + (voxel_grid.voxel_length/2) + voxel_grid.grid_position_x;
    float coordinate_y = (voxel_grid.chunk_size*chunk_position_y * voxel_grid.voxel_length) + (voxel_grid.voxel_length/2) + voxel_grid.grid_position_y;
    float coordinate_z = (voxel_grid.chunk_size*chunk_position_z * voxel_grid.voxel_length) + (voxel_grid.voxel_length/2) + voxel_grid.grid_position_z;

    // Format data to fit the instance VBO
    float voxel_value = voxel_grid.voxel_array[global_index];
    float4 instance_data = make_float4(coordinate_x, coordinate_y, coordinate_z, voxel_value);
    float4* vbo_pointer = reinterpret_cast<float4*>(opengl_VBO); 

    vbo_pointer[global_index] = instance_data;
}

extern "C"{namespace VoxelP{
    void convertToVBO(VoxelGrid &voxel_grid, float* opengl_VBO){
        dim3 block_size = dim3(voxel_grid.chunk_size, voxel_grid.chunk_size, voxel_grid.chunk_size);
        dim3 grid_size = dim3(sizeof(voxel_grid.chunk_array) / 4);

        hipLaunchKernelGGL(convertVoxelToVBOKernel, grid_size, block_size, 0, 0, voxel_grid, opengl_VBO);

        hipDeviceSynchronize();
    }
}}