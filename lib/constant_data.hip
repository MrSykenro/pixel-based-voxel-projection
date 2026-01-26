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

extern "C" void VoxelP::setCameraConstants(const CameraData* camera_data_array, int camera_count){
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
