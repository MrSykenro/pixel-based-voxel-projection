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

