#ifndef MD5PARRAL_H
#define MD5PARRAL_H
#include <string>
#include <arm_neon.h>
#include <cstdint>

typedef unsigned char Byte;
typedef unsigned int bit32;

// 假设已定义的依赖函数
inline uint32x4_t vector_F(uint32x4_t x, uint32x4_t y, uint32x4_t z);
inline uint32x4_t vector_G(uint32x4_t x, uint32x4_t y, uint32x4_t z);
inline uint32x4_t vector_H(uint32x4_t x, uint32x4_t y, uint32x4_t z);
inline uint32x4_t vector_I(uint32x4_t x, uint32x4_t y, uint32x4_t z);
inline uint32x4_t vector_ROTATELEFT(uint32x4_t x, uint32_t s);

// vector_FF（之前已转换为函数）
inline void vector_FF(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    uint32x4_t temp = vector_F(b, c, d);
    temp = vaddq_u32(temp, x);
    temp = vaddq_u32(temp, vdupq_n_u32(ac));
    a = vaddq_u32(a, temp);
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

// vector_GG
inline void vector_GG(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    uint32x4_t temp = vector_G(b, c, d);
    temp = vaddq_u32(temp, x);
    temp = vaddq_u32(temp, vdupq_n_u32(ac));
    a = vaddq_u32(a, temp);
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

// vector_HH
inline void vector_HH(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    uint32x4_t temp = vector_H(b, c, d);
    temp = vaddq_u32(temp, x);
    temp = vaddq_u32(temp, vdupq_n_u32(ac));
    a = vaddq_u32(a, temp);
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

// vector_II
inline void vector_II(uint32x4_t &a, const uint32x4_t &b, const uint32x4_t &c, const uint32x4_t &d,
                      uint32x4_t x, uint32_t s, uint32_t ac) {
    uint32x4_t temp = vector_I(b, c, d);
    temp = vaddq_u32(temp, x);
    temp = vaddq_u32(temp, vdupq_n_u32(ac));
    a = vaddq_u32(a, temp);
    a = vector_ROTATELEFT(a, s);
    a = vaddq_u32(a, b);
}

void vector_MD5Hash(std::string inputs[], bit32 states[][4]);

#endif