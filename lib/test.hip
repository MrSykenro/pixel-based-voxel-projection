#include <test.hpp>
#include <hip/hip_runtime.h>
#include <iostream>

__global__ void vectorAddKernel(const float* A, const float* B, float* C, int N) {
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i < N) C[i] = A[i] + B[i];
}

void run_vector_add(const std::vector<float>& h_A, const std::vector<float>& h_B, std::vector<float>& h_C, int N) {
    float *d_A, *d_B, *d_C;
    size_t size = N * sizeof(float);

    HIP_CHECK(hipMalloc(&d_A, size));
    HIP_CHECK(hipMalloc(&d_B, size));
    HIP_CHECK(hipMalloc(&d_C, size));

    HIP_CHECK(hipMemcpy(d_A, h_A.data(), size, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(d_B, h_B.data(), size, hipMemcpyHostToDevice));

    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) /  threadsPerBlock;

    hipLaunchKernelGGL(vectorAddKernel, dim3(blocksPerGrid), dim3(threadsPerBlock), 0, 0, d_A, d_B, d_C, N);

    HIP_CHECK(hipMemcpy(h_C.data(), d_C, size, hipMemcpyDeviceToHost));

    hipFree(d_A); hipFree(d_B); hipFree(d_C);
}
void copy_to_device_array(const float *h_array, float *d_array, int N){
    HIP_CHECK(hipMemcpy(d_array, h_array, sizeof(float)*N, hipMemcpyHostToDevice));
}

void delete_device_array(float *d_array){
    if(d_array != nullptr){
        HIP_CHECK(hipFree(&d_array));
        d_array = nullptr;
    }
}