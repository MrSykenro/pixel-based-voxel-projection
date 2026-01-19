#include <iostream>
#include <vector>
#include "test.hpp"
#include "voxel_projection.hpp"

int main() {
    int N = 100;
    std::vector<float> a(N, 10.0f), b(N, 2.0f), c(N, 0.0f);

    // Call the wrapper
    run_vector_add(a, b, c, N);
    
    std::cout << "Result of first element: " << c[0] << std::endl;
    return 0;
}