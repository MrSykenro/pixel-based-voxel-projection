// 3x3 Matrix of floats stored as an array of 9 floats
struct Matrix3x3f
{
    float matrix[9];
};

// Camera information for transferring into device
struct CameraData
{
    int camera_index;
    float fov_degrees;
    float rot_x, rot_y, rot_z; 
    float pos_x, pos_y, pos_z; 
};

// A ray buffer to pass array pointers that hold individual components of rays
struct RayBuffer
{
    float* dir_x;
    float* dir_y;
    float* dir_z;
    float* intensity;
    int* camera_id;
    int* ray_count; // THIS IS A SINGLE INTEGER. EDIT USING ATOMIC FUNCTIONS ONLY.
    size_t max_ray_count;
};


struct VoxelGrid
{
    int voxel_length;
    int chunk_size;

    float grid_position_x;
    float grid_position_y;
    float grid_position_z;

    int chunk_grid_x; 
    int chunk_grid_y; 
    int chunk_grid_z; 

    int* active_chunks;
    int* chunk_mask;
    float* voxel_array;
};
