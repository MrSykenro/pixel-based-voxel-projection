#include <iostream>
#include <vector>
#include "test.hpp"
#include "voxel_projection.hpp"

int main() {
    // -------------------------------------------------------------------------
    // For Testing - Unrelated
    // -------------------------------------------------------------------------

    int N = 100;
    std::vector<float> a(N, 10.0f), b(N, 2.0f), c(N, 0.0f);

    // Call the wrapper
    run_vector_add(a, b, c, N);

    std::cout << "Result of first element: " << c[0] << std::endl;

    // -------------------------------------------------------------------------
    // Pixel-Based-Voxel-Projection
    // -------------------------------------------------------------------------

    // Fake Data
    // Needs a way to take frames
    CameraData camera_data[1] = {{1, 60.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f}};
    int image_width = 1920;
    int image_height = 1080;
    int camera_count = 1;
    float threshold = 0.1;
    
    float* difference_image = nullptr; // Placeholder

    int pixel_count = image_height * image_width * camera_count;

    VoxelP::setCameraConstants(camera_data, 1);

    RayBuffer ray_buffer = VoxelP::initRayBuffer(pixel_count);

    VoxelP::generateRays(difference_image, ray_buffer, threshold, image_width, image_height, camera_count);

    VoxelP::freeRayBuffer(ray_buffer);

    return 0;
}