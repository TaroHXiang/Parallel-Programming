#include <iostream>
#include <string>
#include <cstring>
#include <arm_neon.h>
#include <iomanip>
#include <assert.h>
#include <chrono>
#include <algorithm>
using namespace std;
using namespace chrono;

typedef unsigned char Byte;
typedef unsigned int bit32;

const int NUM_MESSAGES = 8; // 批处理 8 个输入

#define vector_F(x, y, z) vorrq_u32(vandq_u32(x, y), vandq_u32(vmvnq_u32(x), z))
#define vector_G(x, y, z) vorrq_u32(vandq_u32(x, z), vandq_u32(y, vmvnq_u32(z)))
#define vector_H(x, y, z) veorq_u32(veorq_u32(x, y), z)
#define vector_I(x, y, z) veorq_u32(y, vorrq_u32(x, vmvnq_u32(z)))

// 修复：恢复移位实现，支持动态 n
#define vector_ROTATELEFT(x, n) vorrq_u32(vshlq_n_u32(x, n), vshrq_n_u32(x, 32 - n))

inline void vector_FF(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    a = vaddq_u32(a, vaddq_u32(vaddq_u32(vector_F(b, c, d), x), vdupq_n_u32(ac)));
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

inline void vector_GG(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    a = vaddq_u32(a, vaddq_u32(vaddq_u32(vector_G(b, c, d), x), vdupq_n_u32(ac)));
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

inline void vector_HH(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    a = vaddq_u32(a, vaddq_u32(vaddq_u32(vector_H(b, c, d), x), vdupq_n_u32(ac)));
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

inline void vector_II(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    a = vaddq_u32(a, vaddq_u32(vaddq_u32(vector_I(b, c, d), x), vdupq_n_u32(ac)));
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

void MD5Hash(string input, bit32 *state);
void vector_MD5Hash(string input[], bit32 state[][4]);