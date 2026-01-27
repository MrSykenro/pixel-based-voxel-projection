#pragma once
#define _USE_MATH_DEFINES

#include "formats.hpp"
#include <iostream>
#include <math.h>

#ifdef __HIPCC__
#include <hip/hip_runtime.h>
extern __constant__ Matrix3x3f c_camera_matrices[32];
extern __constant__ float3 c_camera_positions[32];
extern __constant__ float c_focal_lengths[32];
#endif

namespace VoxelP{ extern "C"{    
/**
 * Sets the camera data into constant memory.
 * The array is limited by 32 elements (cameras).
 * @param camera_data_array The CameraData struct array containing information of all the cameras
 * @param camera_count The number of that cameras are included in the array. (Limited by 32)
 */
void setCameraConstants(const CameraData* camera_data_array, int camera_count);
}}
