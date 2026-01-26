#include <iostream>
#include <vector>
#include "test.hpp"
#include "voxel_projection.hpp"

int main() {
    int N = 100;
    std::vector<float> a(N, 10.0f), b(N, 2.0f), c(N, 0.0f);

    // Call the wrapper
    run_vector_add(a, b, c, N);

    CameraData camera_data[1] = {{1, 60.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f}};

    VoxelP::setCameraConstants(camera_data, 1);
    
    std::cout << "Result of first element: " << c[0] << std::endl;
    return 0;
}