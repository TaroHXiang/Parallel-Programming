初始版SIMD实现在md5parral.cpp和md5parral.h文件中，配套的main函数为main1.cpp。
为了实现对串行算法的加速，使算法在SIMD实现下也能比串行算法更快，进行了4次改进，每一次都为一个版本，最后实现了加速。第一版在md5parral.cpp和md5parral.h文件中，第二版在md5parral2.cpp和md5parral2.h文件中，第三版在md5parral3&.cpp和md5parral3&.h文件中（实现了对传入参数的改进，变为引用），第四版在md5parral4(&+adv).cpp和md5parral4(&+adv).h中（加入了参数引用和内存优化）。
