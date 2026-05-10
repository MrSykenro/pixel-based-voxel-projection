#ifndef TEST
#define TEST

#include <vector>

#define HIP_CHECK(command) { \
    hipError_t status = command; \
    if (status != hipSuccess) { \
        std::cerr << "HIP Error: " << hipGetErrorString(status) << std::endl; \
        exit(1); \
    } \
}

// This is the wrapper function that will be called from main.cpp
void run_vector_add(
    const std::vector<float>& h_A, 
    const std::vector<float>& h_B, 
    std::vector<float>& h_C, 
    int N
);

void copy_to_device_array(const float *h_array, float *d_array, int N);
void delete_device_array(float *d_array);
#endif