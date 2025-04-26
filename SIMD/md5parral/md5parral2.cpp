#include "md5parral2.h"
#include <iomanip>
#include <assert.h>
#include <chrono>
#include <arm_neon.h>
#include <algorithm>
using namespace std;
using namespace chrono;

void StringProcess(const string& input, Byte* paddedMessage, int* n_byte) {
    Byte* blocks = (Byte*)input.c_str();
    int length = input.length();
    int bitLength = length * 8;

    int paddingBits = bitLength % 512;
    if (paddingBits > 448) {
        paddingBits += 512 - (paddingBits - 448);
    } else if (paddingBits < 448) {
        paddingBits = 448 - paddingBits;
    } else if (paddingBits == 448) {
        paddingBits = 512;
    }

    int paddingBytes = paddingBits / 8;
    int paddedLength = length + paddingBytes + 8;

    if (paddedLength > 1024) {
        cerr << "Input too long for stack buffer" << endl;
        exit(1);
    }

    memcpy(paddedMessage, blocks, length);
    paddedMessage[length] = 0x80;
    memset(paddedMessage + length + 1, 0, paddingBytes - 1);

    for (int i = 0; i < 8; ++i) {
        paddedMessage[length + paddingBytes + i] = ((uint64_t)length * 8 >> (i * 8)) & 0xFF;
    }

    int residual = 8 * paddedLength % 512;
    assert(residual == 0);

    *n_byte = paddedLength;
}

void vector_MD5Hash(string inputs[], bit32 states[][4]) {
    alignas(16) Byte paddedMessages[NUM_MESSAGES][1024];
    int messageLengths[NUM_MESSAGES];

    for (int i = 0; i < NUM_MESSAGES; i++) {
        StringProcess(inputs[i], paddedMessages[i], &messageLengths[i]);
        if (messageLengths[i] > 64) {
            cerr << "Input too long, max_blocks > 1 not supported" << endl;
            exit(1);
        }
    }

    alignas(16) uint32x4_t state[4];
    state[0] = vdupq_n_u32(0x67452301);
    state[1] = vdupq_n_u32(0xefcdab89);
    state[2] = vdupq_n_u32(0x98badcfe);
    state[3] = vdupq_n_u32(0x10325476);

    alignas(16) uint32x4_t x[16];
    for (int i1 = 0; i1 < 16; i1++) {
        alignas(16) uint32_t values[NUM_MESSAGES];
        for (int i = 0; i < NUM_MESSAGES; i++) {
            if (4 * i1 < messageLengths[i]) {
                values[i] = *reinterpret_cast<uint32_t*>(&paddedMessages[i][4 * i1]);
            } else {
                values[i] = 0;
            }
        }
        x[i1] = vld1q_u32(values);
    }

    alignas(16) uint32x4_t a = state[0], b = state[1], c = state[2], d = state[3];

    // 完全展开所有轮
    // 第一轮
    vector_FF(a, b, c, d, x[0], 7, 0xd76aa478);
    vector_FF(d, a, b, c, x[1], 12, 0xe8c7b756);
    vector_FF(c, d, a, b, x[2], 17, 0x242070db);
    vector_FF(b, c, d, a, x[3], 22, 0xc1bdceee);
    vector_FF(a, b, c, d, x[4], 7, 0xf57c0faf);
    vector_FF(d, a, b, c, x[5], 12, 0x4787c62a);
    vector_FF(c, d, a, b, x[6], 17, 0xa8304613);
    vector_FF(b, c, d, a, x[7], 22, 0xfd469501);
    vector_FF(a, b, c, d, x[8], 7, 0x698098d8);
    vector_FF(d, a, b, c, x[9], 12, 0x8b44f7af);
    vector_FF(c, d, a, b, x[10], 17, 0xffff5bb1);
    vector_FF(b, c, d, a, x[11], 22, 0x895cd7be);
    vector_FF(a, b, c, d, x[12], 7, 0x6b901122);
    vector_FF(d, a, b, c, x[13], 12, 0xfd987193);
    vector_FF(c, d, a, b, x[14], 17, 0xa679438e);
    vector_FF(b, c, d, a, x[15], 22, 0x49b40821);

    // 第二轮
    vector_GG(a, b, c, d, x[1], 5, 0xf61e2562);
    vector_GG(d, a, b, c, x[6], 9, 0xc040b340);
    vector_GG(c, d, a, b, x[11], 14, 0x265e5a51);
    vector_GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
    vector_GG(a, b, c, d, x[5], 5, 0xd62f105d);
    vector_GG(d, a, b, c, x[10], 9, 0x02441453);
    vector_GG(c, d, a, b, x[15], 14, 0xd8a1e681);
    vector_GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
    vector_GG(a, b, c, d, x[9], 5, 0x21e1cde6);
    vector_GG(d, a, b, c, x[14], 9, 0xc33707d6);
    vector_GG(c, d, a, b, x[3], 14, 0xf4d50d87);
    vector_GG(b, c, d, a, x[8], 20, 0x455a14ed);
    vector_GG(a, b, c, d, x[13], 5, 0xa9e3e905);
    vector_GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
    vector_GG(c, d, a, b, x[7], 14, 0x676f02d9);
    vector_GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    // 第三轮
    vector_HH(a, b, c, d, x[5], 4, 0xfffa3942);
    vector_HH(d, a, b, c, x[8], 11, 0x8771f681);
    vector_HH(c, d, a, b, x[11], 16, 0x6d9d6122);
    vector_HH(b, c, d, a, x[14], 23, 0xfde5380c);
    vector_HH(a, b, c, d, x[1], 4, 0xa4beea44);
    vector_HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
    vector_HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
    vector_HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    vector_HH(a, b, c, d, x[13], 4, 0x289b7ec6);
    vector_HH(d, a, b, c, x[0], 11, 0xeaa127fa);
    vector_HH(c, d, a, b, x[3], 16, 0xd4ef3085);
    vector_HH(b, c, d, a, x[6], 23, 0x04881d05);
    vector_HH(a, b, c, d, x[9], 4, 0xd9d4d039);
    vector_HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    vector_HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
    vector_HH(b, c, d, a, x[2], 23, 0xc4ac5665);

    // 第四轮
    vector_II(a, b, c, d, x[0], 6, 0xf4292244);
    vector_II(d, a, b, c, x[7], 10, 0x432aff97);
    vector_II(c, d, a, b, x[14], 15, 0xab9423a7);
    vector_II(b, c, d, a, x[5], 21, 0xfc93a039);
    vector_II(a, b, c, d, x[12], 6, 0x655b59c3);
    vector_II(d, a, b, c, x[3], 10, 0x8f0ccc92);
    vector_II(c, d, a, b, x[10], 15, 0xffeff47d);
    vector_II(b, c, d, a, x[1], 21, 0x85845dd1);
    vector_II(a, b, c, d, x[8], 6, 0x6fa87e4f);
    vector_II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    vector_II(c, d, a, b, x[6], 15, 0xa3014314);
    vector_II(b, c, d, a, x[13], 21, 0x4e0811a1);
    vector_II(a, b, c, d, x[4], 6, 0xf7537e82);
    vector_II(d, a, b, c, x[11], 10, 0xbd3af235);
    vector_II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
    vector_II(b, c, d, a, x[9], 21, 0xeb86d391);

    state[0] = vaddq_u32(state[0], a);
    state[1] = vaddq_u32(state[1], b);
    state[2] = vaddq_u32(state[2], c);
    state[3] = vaddq_u32(state[3], d);

    for (int i = 0; i < 4; i++) {
        alignas(16) uint32_t values[NUM_MESSAGES];
        vst1q_u32(values, state[i]);
        for (int j = 0; j < NUM_MESSAGES; j++) {
            uint32_t value = values[j];
            states[j][i] = ((value & 0xff) << 24) |
                           ((value & 0xff00) << 8) |
                           ((value & 0xff0000) >> 8) |
                           ((value & 0xff000000) >> 24);
        }
    }
}