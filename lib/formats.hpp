struct Vector3f
{
    float vector[3];
};

struct Matrix3x3f
{
    float matrix[9];
};

struct CameraData
{
    int camera_index;
    float fov_degrees;
    float rot_x, rot_y, rot_z; 
    float pos_x, pos_y, pos_z; 
};

