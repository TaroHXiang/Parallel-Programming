#include "md5parral.h"
#include <iomanip>
#include <assert.h>
#include <chrono>
#include <arm_neon.h>
#include <algorithm> 
using namespace std;
using namespace chrono;
const int NUM_MESSAGES = 4;
/**
 * StringProcess: 将单个输入字符串转换成MD5计算所需的消息数组
 * @param input 输入
 * @param[out] n_byte 用于给调用者传递额外的返回值，即最终Byte数组的长度
 * @return Byte消息数组
 */
Byte *StringProcess(string input, int *n_byte)
{
	// 将输入的字符串转换为Byte为单位的数组
	Byte *blocks = (Byte *)input.c_str();
	int length = input.length();

	// 计算原始消息长度（以比特为单位）
	int bitLength = length * 8;

	// paddingBits: 原始消息需要的padding长度（以bit为单位）
	// 对于给定的消息，将其补齐至length%512==448为止
	// 需要注意的是，即便给定的消息满足length%512==448，也需要再pad 512bits
	int paddingBits = bitLength % 512;
	if (paddingBits > 448)
	{
		paddingBits += 512 - (paddingBits - 448);
	}
	else if (paddingBits < 448)
	{
		paddingBits = 448 - paddingBits;
	}
	else if (paddingBits == 448)
	{
		paddingBits = 512;
	}

	// 原始消息需要的padding长度（以Byte为单位）
	int paddingBytes = paddingBits / 8;
	// 创建最终的字节数组
	// length + paddingBytes + 8:
	// 1. length为原始消息的长度（bits）
	// 2. paddingBytes为原始消息需要的padding长度（Bytes）
	// 3. 在pad到length%512==448之后，需要额外附加64bits的原始消息长度，即8个bytes
	int paddedLength = length + paddingBytes + 8;
	Byte *paddedMessage = new Byte[paddedLength];

	// 复制原始消息
	memcpy(paddedMessage, blocks, length);

	// 添加填充字节。填充时，第一位为1，后面的所有位均为0。
	// 所以第一个byte是0x80
	paddedMessage[length] = 0x80;							 // 添加一个0x80字节
	memset(paddedMessage + length + 1, 0, paddingBytes - 1); // 填充0字节

	// 添加消息长度（64比特，小端格式）
	for (int i = 0; i < 8; ++i)
	{
		// 特别注意此处应当将bitLength转换为uint64_t
		// 这里的length是原始消息的长度
		paddedMessage[length + paddingBytes + i] = ((uint64_t)length * 8 >> (i * 8)) & 0xFF;
	}

	// 验证长度是否满足要求。此时长度应当是512bit的倍数
	int residual = 8 * paddedLength % 512;
	// assert(residual == 0);

	// 在填充+添加长度之后，消息被分为n_blocks个512bit的部分
	*n_byte = paddedLength;
	return paddedMessage;
}


/**
 * MD5Hash: 将单个输入字符串转换成MD5
 * @param input 输入
 * @param[out] state 用于给调用者传递额外的返回值，即最终的缓冲区，也就是MD5的结果
 * @return Byte消息数组
 */
void vector_MD5Hash(string inputs[], bit32 states[][4]) {
    Byte *paddedMessages[NUM_MESSAGES];
    int messageLengths[NUM_MESSAGES];
    int max_blocks = 0;

    // 填充每个消息
    for (int i = 0; i < NUM_MESSAGES; i++) {
        paddedMessages[i] = StringProcess(inputs[i], &messageLengths[i]);
        max_blocks = max(max_blocks, messageLengths[i] / 64);
    }

    // 初始化状态
    uint32x4_t state[4];
    state[0] = vdupq_n_u32(0x67452301);
    state[1] = vdupq_n_u32(0xefcdab89);
    state[2] = vdupq_n_u32(0x98badcfe);
    state[3] = vdupq_n_u32(0x10325476);

    // 分块处理
    for (int block = 0; block < max_blocks; block++) {
        uint32x4_t x[16];
        // 加载 NUM_MESSAGES 个消息的当前块
        for (int i1 = 0; i1 < 16; i1++) {
            uint32_t values[NUM_MESSAGES];
            for (int i = 0; i < NUM_MESSAGES; i++) {
                if (block < messageLengths[i] / 64) {
                    values[i] = (paddedMessages[i][4 * i1 + block * 64]) |
                                (paddedMessages[i][4 * i1 + 1 + block * 64] << 8) |
                                (paddedMessages[i][4 * i1 + 2 + block * 64] << 16) |
                                (paddedMessages[i][4 * i1 + 3 + block * 64] << 24);
                } else {
                    values[i] = 0; // 短消息填充零
                }
            }
            x[i1] = vld1q_u32(values); // 加载 4 个 32 位值
        }

        uint32x4_t a = state[0], b = state[1], c = state[2], d = state[3];

        // 第一轮
        vector_FF(a, b, c, d, x[0], s11, 0xd76aa478);
        vector_FF(d, a, b, c, x[1], s12, 0xe8c7b756);
        vector_FF(c, d, a, b, x[2], s13, 0x242070db);
        vector_FF(b, c, d, a, x[3], s14, 0xc1bdceee);
        vector_FF(a, b, c, d, x[4], s11, 0xf57c0faf);
        vector_FF(d, a, b, c, x[5], s12, 0x4787c62a);
        vector_FF(c, d, a, b, x[6], s13, 0xa8304613);
        vector_FF(b, c, d, a, x[7], s14, 0xfd469501);
        vector_FF(a, b, c, d, x[8], s11, 0x698098d8);
        vector_FF(d, a, b, c, x[9], s12, 0x8b44f7af);
        vector_FF(c, d, a, b, x[10], s13, 0xffff5bb1);
        vector_FF(b, c, d, a, x[11], s14, 0x895cd7be);
        vector_FF(a, b, c, d, x[12], s11, 0x6b901122);
        vector_FF(d, a, b, c, x[13], s12, 0xfd987193);
        vector_FF(c, d, a, b, x[14], s13, 0xa679438e);
        vector_FF(b, c, d, a, x[15], s14, 0x49b40821);

        // 第二轮
        vector_GG(a, b, c, d, x[1], s21, 0xf61e2562);
        vector_GG(d, a, b, c, x[6], s22, 0xc040b340);
        vector_GG(c, d, a, b, x[11], s23, 0x265e5a51);
        vector_GG(b, c, d, a, x[0], s24, 0xe9b6c7aa);
        vector_GG(a, b, c, d, x[5], s21, 0xd62f105d);
        vector_GG(d, a, b, c, x[10], s22, 0x02441453);
        vector_GG(c, d, a, b, x[15], s23, 0xd8a1e681);
        vector_GG(b, c, d, a, x[4], s24, 0xe7d3fbc8);
        vector_GG(a, b, c, d, x[9], s21, 0x21e1cde6);
        vector_GG(d, a, b, c, x[14], s22, 0xc33707d6);
        vector_GG(c, d, a, b, x[3], s23, 0xf4d50d87);
        vector_GG(b, c, d, a, x[8], s24, 0x455a14ed);
        vector_GG(a, b, c, d, x[13], s21, 0xa9e3e905);
        vector_GG(d, a, b, c, x[2], s22, 0xfcefa3f8);
        vector_GG(c, d, a, b, x[7], s23, 0x676f02d9);
        vector_GG(b, c, d, a, x[12], s24, 0x8d2a4c8a);

        // 第三轮
        vector_HH(a, b, c, d, x[5], s31, 0xfffa3942);
        vector_HH(d, a, b, c, x[8], s32, 0x8771f681);
        vector_HH(c, d, a, b, x[11], s33, 0x6d9d6122);
        vector_HH(b, c, d, a, x[14], s34, 0xfde5380c);
        vector_HH(a, b, c, d, x[1], s31, 0xa4beea44);
        vector_HH(d, a, b, c, x[4], s32, 0x4bdecfa9);
        vector_HH(c, d, a, b, x[7], s33, 0xf6bb4b60);
        vector_HH(b, c, d, a, x[10], s34, 0xbebfbc70);
        vector_HH(a, b, c, d, x[13], s31, 0x289b7ec6);
        vector_HH(d, a, b, c, x[0], s32, 0xeaa127fa);
        vector_HH(c, d, a, b, x[3], s33, 0xd4ef3085);
        vector_HH(b, c, d, a, x[6], s34, 0x04881d05);
        vector_HH(a, b, c, d, x[9], s31, 0xd9d4d039);
        vector_HH(d, a, b, c, x[12], s32, 0xe6db99e5);
        vector_HH(c, d, a, b, x[15], s33, 0x1fa27cf8);
        vector_HH(b, c, d, a, x[2], s34, 0xc4ac5665);

        // 第四轮
        vector_II(a, b, c, d, x[0], s41, 0xf4292244);
        vector_II(d, a, b, c, x[7], s42, 0x432aff97);
        vector_II(c, d, a, b, x[14], s43, 0xab9423a7);
        vector_II(b, c, d, a, x[5], s44, 0xfc93a039);
        vector_II(a, b, c, d, x[12], s41, 0x655b59c3);
        vector_II(d, a, b, c, x[3], s42, 0x8f0ccc92);
        vector_II(c, d, a, b, x[10], s43, 0xffeff47d);
        vector_II(b, c, d, a, x[1], s44, 0x85845dd1);
        vector_II(a, b, c, d, x[8], s41, 0x6fa87e4f);
        vector_II(d, a, b, c, x[15], s42, 0xfe2ce6e0);
        vector_II(c, d, a, b, x[6], s43, 0xa3014314);
        vector_II(b, c, d, a, x[13], s44, 0x4e0811a1);
        vector_II(a, b, c, d, x[4], s41, 0xf7537e82);
        vector_II(d, a, b, c, x[11], s42, 0xbd3af235);
        vector_II(c, d, a, b, x[2], s43, 0x2ad7d2bb);
        vector_II(b, c, d, a, x[9], s44, 0xeb86d391);

        // 更新状态
        state[0] = vaddq_u32(state[0], a);
        state[1] = vaddq_u32(state[1], b);
        state[2] = vaddq_u32(state[2], c);
        state[3] = vaddq_u32(state[3], d);
    }

    // 字节序调整并存储结果
    for (int i = 0; i < 4; i++) {
        uint32_t values[NUM_MESSAGES];
        vst1q_u32(values, state[i]);
        for (int j = 0; j < NUM_MESSAGES; j++) {
            uint32_t value = values[j];
            states[j][i] = ((value & 0xff) << 24) |
                           ((value & 0xff00) << 8) |
                           ((value & 0xff0000) >> 8) |
                           ((value & 0xff000000) >> 24);
        }
    }

    // 清理内存
    for (int i = 0; i < NUM_MESSAGES; i++) {
        delete[] paddedMessages[i];
    }
}
