#include <cuda_runtime.h>
#include <cstring>
#ifndef MAX_LEN
#define MAX_LEN 64
#endif

extern "C" __global__ void generate_single_segment_kernel(
    const char** d_values, char** d_out, int count)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;

    const char* src = d_values[idx];
    char* dst = d_out[idx];
    // 简单 strcpy；假设长度已 < MAX_LEN
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}


extern "C" __global__ void generate_with_prefix_kernel(
    const char* d_prefix, int prefixLen,
    const char** d_values, char** d_out, int count)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;

    char* dst = d_out[idx];

    // 1. 复制 prefix
    for (int i = 0; i < prefixLen; ++i) {
        dst[i] = d_prefix[i];
    }

    // 2. 追加 value
    const char* src = d_values[idx];
    int j = 0;
    while (src[j] != '\0') {
        dst[prefixLen + j] = src[j];
        ++j;
    }
    dst[prefixLen + j] = '\0';
}
