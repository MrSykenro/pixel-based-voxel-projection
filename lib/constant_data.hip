#include "voxel_projection.hpp"
#include "formats.hpp"
#include <hip/hip_runtime.h>
#include <hip/hip_vector_types.h>
#include <iostream>

__constant__ Matrix3x3f c_camera_matrices[32];
__constant__ float3 c_camera_positions[32];
__constant__ float c_focal_lengths[32];

extern "C" Matrix3x3f calculateRotationMatrix(float x, float y, float z){
    
}

extern "C" void setCameraConstants(CameraData* camera_data_array, int camera_count){
    for(int i = 0; i < camera_count; i++){
        CameraData camera_data = camera_data_array[i];
        
    }
}
