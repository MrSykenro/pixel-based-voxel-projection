#ifndef TEST
#define TEST

#include <vector>

// This is the wrapper function that will be called from main.cpp
void run_vector_add(
    const std::vector<float>& h_A, 
    const std::vector<float>& h_B, 
    std::vector<float>& h_C, 
    int N
);

void copy_array_host_to_device(const float &h_array, int N, float &d_array);
void delete_device_array(float *d_array);
#endif