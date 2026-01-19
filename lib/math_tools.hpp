#include "voxel_projection.hpp"
#include <hip/hip_runtime.h>
#include <hip/hip_vector_types.h>
#include <iostream>
#include <cfloat>
#include <math.h>

static constexpr float EPSILON = 1e-12f;

__device__ inline float safeDivision(float numerator, float denominator){
    if(fabsf(denominator) < EPSILON) return FLT_MAX;
    return fdividef(numerator, denominator);
}

__device__ inline float3 safeDivisonFloat3(const float3 &numerator, const float3 &denominator){
    return make_float3(
        safeDivision(numerator.x, denominator.x),
        safeDivision(numerator.y, denominator.y),
        safeDivision(numerator.z, denominator.z)
    );
}

__device__ inline float3 absoluteFloat3(const float3 &vector){
    return make_float3(
        fabsf(vector.x),
        fabsf(vector.y),
        fabsf(vector.z)
    );
}

__device__ inline float3 normalizeVector(const float3 &vector){
    float squared_magnitude = vector.x*vector.x + vector.y*vector.y + vector.z*vector.z;
    float inverse_square_root = rsqrtf(squared_magnitude);
    float3 result = make_float3(
        vector.x * inverse_square_root,
        vector.y * inverse_square_root,
        vector.z * inverse_square_root
    );
    return result;
}