#include "PCFG.h"
#include <cuda_runtime.h>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
using namespace std;

#ifdef __CUDACC__
extern "C" __global__ void generate_single_segment_kernel(const char** d_values,
                                                         char** d_out,
                                                         int     count);
extern "C" __global__ void generate_with_prefix_kernel(const char* d_prefix,
                                                        int       prefixLen,
                                                        const char** d_values,
                                                        char**   d_out,
                                                        int      count);
#endif

#ifndef MAX_LEN
#define MAX_LEN 64   // 单条口令最大长度（含前缀+value+\0）
#endif

#define CUDA_CHK(call)                                                       \
    do {                                                                     \
        cudaError_t e = (call);                                              \
        if (e != cudaSuccess) {                                              \
            std::cerr << "CUDA Error: " << cudaGetErrorString(e)            \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error("CUDA failure");                       \
        }                                                                    \
    } while (0)
void PriorityQueue::CalProb(PT &pt)
{
    // 计算PriorityQueue里面一个PT的流程如下：
    // 1. 首先需要计算一个PT本身的概率。例如，L6S1的概率为0.15
    // 2. 需要注意的是，Queue里面的PT不是“纯粹的”PT，而是除了最后一个segment以外，全部被value实例化的PT
    // 3. 所以，对于L6S1而言，其在Queue里面的实际PT可能是123456S1，其中“123456”为L6的一个具体value。
    // 4. 这个时候就需要计算123456在L6中出现的概率了。假设123456在所有L6 segment中的概率为0.1，那么123456S1的概率就是0.1*0.15

    // 计算一个PT本身的概率。后续所有具体segment value的概率，直接累乘在这个初始概率值上
    pt.prob = pt.preterm_prob;

    // index: 标注当前segment在PT中的位置
    int index = 0;


    for (int idx : pt.curr_indices)
    {
        // pt.content[index].PrintSeg();
        if (pt.content[index].type == 1)
        {
            // 下面这行代码的意义：
            // pt.content[index]：目前需要计算概率的segment
            // m.FindLetter(seg): 找到一个letter segment在模型中的对应下标
            // m.letters[m.FindLetter(seg)]：一个letter segment在模型中对应的所有统计数据
            // m.letters[m.FindLetter(seg)].ordered_values：一个letter segment在模型中，所有value的总数目
            pt.prob *= m.letters[m.FindLetter(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.letters[m.FindLetter(pt.content[index])].total_freq;
            // cout << m.letters[m.FindLetter(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.letters[m.FindLetter(pt.content[index])].total_freq << endl;
        }
        if (pt.content[index].type == 2)
        {
            pt.prob *= m.digits[m.FindDigit(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.digits[m.FindDigit(pt.content[index])].total_freq;
            // cout << m.digits[m.FindDigit(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.digits[m.FindDigit(pt.content[index])].total_freq << endl;
        }
        if (pt.content[index].type == 3)
        {
            pt.prob *= m.symbols[m.FindSymbol(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.symbols[m.FindSymbol(pt.content[index])].total_freq;
            // cout << m.symbols[m.FindSymbol(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.symbols[m.FindSymbol(pt.content[index])].total_freq << endl;
        }
        index += 1;
    }
    // cout << pt.prob << endl;
}

void PriorityQueue::init()
{
    // cout << m.ordered_pts.size() << endl;
    // 用所有可能的PT，按概率降序填满整个优先队列
    for (PT pt : m.ordered_pts)
    {
        for (segment seg : pt.content)
        {
            if (seg.type == 1)
            {
                // 下面这行代码的意义：
                // max_indices用来表示PT中各个segment的可能数目。例如，L6S1中，假设模型统计到了100个L6，那么L6对应的最大下标就是99
                // （但由于后面采用了"<"的比较关系，所以其实max_indices[0]=100）
                // m.FindLetter(seg): 找到一个letter segment在模型中的对应下标
                // m.letters[m.FindLetter(seg)]：一个letter segment在模型中对应的所有统计数据
                // m.letters[m.FindLetter(seg)].ordered_values：一个letter segment在模型中，所有value的总数目
                pt.max_indices.emplace_back(m.letters[m.FindLetter(seg)].ordered_values.size());
            }
            if (seg.type == 2)
            {
                pt.max_indices.emplace_back(m.digits[m.FindDigit(seg)].ordered_values.size());
            }
            if (seg.type == 3)
            {
                pt.max_indices.emplace_back(m.symbols[m.FindSymbol(seg)].ordered_values.size());
            }
        }
        pt.preterm_prob = float(m.preterm_freq[m.FindPT(pt)]) / m.total_preterm;
        // pt.PrintPT();
        // cout << " " << m.preterm_freq[m.FindPT(pt)] << " " << m.total_preterm << " " << pt.preterm_prob << endl;

        // 计算当前pt的概率
        CalProb(pt);
        // 将PT放入优先队列
        priority.emplace_back(pt);
    }
    // cout << "priority size:" << priority.size() << endl;
}

void PriorityQueue::PopNext(int batch)
{
    if (batch <= 0) batch = 1;
    if (priority.empty())  return;

    // 保障 batch 不超过队列长度
    int take = std::min(batch, static_cast<int>(priority.size()));

    //------------------------------------------------------------------
    // step-1 : 针对 batch 个 PT 逐个 Generate() 生成猜测
    //------------------------------------------------------------------
    for (int t = 0; t < take; ++t) {
        Generate(priority[t]);                // 并行生成猜测
    }

    //------------------------------------------------------------------
    // step-2 : 批量收集新 PT，统一插回优先队列
    //------------------------------------------------------------------
    std::vector<PT> new_pts_all;
    for (int t = 0; t < take; ++t) {
        std::vector<PT> tmp = priority[t].NewPTs();
        new_pts_all.insert(new_pts_all.end(), tmp.begin(), tmp.end());
    }

    // 计算概率并逐一插回（完全沿用原先的插入逻辑）
    for (PT pt : new_pts_all)
    {
        CalProb(pt);
        for (auto iter = priority.begin(); iter != priority.end(); ++iter)
        {
            if (iter != priority.begin() && iter != priority.end() - 1)
            {
                if (pt.prob <= iter->prob && pt.prob > (iter + 1)->prob)
                {
                    priority.emplace(iter + 1, pt);
                    goto inserted;
                }
            }
            if (iter == priority.end() - 1) { priority.emplace_back(pt); goto inserted; }
            if (iter == priority.begin() && iter->prob < pt.prob) {
                priority.emplace(iter, pt);  goto inserted;
            }
        }
    inserted: ;
    }

    //------------------------------------------------------------------
    // step-3 : 删除已经处理完的 batch 个 PT
    //------------------------------------------------------------------
    priority.erase(priority.begin(), priority.begin() + take);
}
// 这个函数你就算看不懂，对并行算法的实现影响也不大
// 当然如果你想做一个基于多优先队列的并行算法，可能得稍微看一看了
vector<PT> PT::NewPTs()
{
    // 存储生成的新PT
    vector<PT> res;

    // 假如这个PT只有一个segment
    // 那么这个segment的所有value在出队前就已经被遍历完毕，并作为猜测输出
    // 因此，所有这个PT可能对应的口令猜测已经遍历完成，无需生成新的PT
    if (content.size() == 1)
    {
        return res;
    }
    else
    {
        // 最初的pivot值。我们将更改位置下标大于等于这个pivot值的segment的值（最后一个segment除外），并且一次只更改一个segment
        // 上面这句话里是不是有没看懂的地方？接着往下看你应该会更明白
        int init_pivot = pivot;

        // 开始遍历所有位置值大于等于init_pivot值的segment
        // 注意i < curr_indices.size() - 1，也就是除去了最后一个segment（这个segment的赋值预留给并行环节）
        for (int i = pivot; i < curr_indices.size() - 1; i += 1)
        {
            // curr_indices: 标记各segment目前的value在模型里对应的下标
            curr_indices[i] += 1;

            // max_indices：标记各segment在模型中一共有多少个value
            if (curr_indices[i] < max_indices[i])
            {
                // 更新pivot值
                pivot = i;
                res.emplace_back(*this);
            }

            // 这个步骤对于你理解pivot的作用、新PT生成的过程而言，至关重要
            curr_indices[i] -= 1;
        }
        pivot = init_pivot;
        return res;
    }

    return res;
}


// 这个函数是PCFG并行化算法的主要载体
// 尽量看懂，然后进行并行实现
void PriorityQueue::Generate(PT pt)
{
    // 1. 计算概率（与并行无关）
    CalProb(pt);

    // ------------------------------------------------------------------
    // 2. 单段 PT：直接并行复制 ordered_values
    // ------------------------------------------------------------------
    if (pt.content.size() == 1)
    {
        // 2.1 取得模型指针
        segment* a = nullptr;
        if (pt.content[0].type == 1) a = &m.letters[m.FindLetter(pt.content[0])];
        if (pt.content[0].type == 2) a = &m.digits [m.FindDigit (pt.content[0])];
        if (pt.content[0].type == 3) a = &m.symbols[m.FindSymbol(pt.content[0])];

        const int N = pt.max_indices[0];
        if (N == 0) return;

#ifdef __CUDACC__
        // ---------------- host 端准备输入指针 ----------------
        vector<const char*> h_valPtrs(N);
        vector<size_t>      h_valLen (N);
        for (int i = 0; i < N; ++i) {
            h_valPtrs[i] = a->ordered_values[i].c_str();
            h_valLen [i] = a->ordered_values[i].size() + 1; // 含 '\0'
        }

        // 为每个 value 分配 device 缓冲并复制
        vector<char*> h_dValuePtrs(N);
        for (int i = 0; i < N; ++i) {
            CUDA_CHK(cudaMalloc(&h_dValuePtrs[i], h_valLen[i]));
            CUDA_CHK(cudaMemcpy(h_dValuePtrs[i], h_valPtrs[i], h_valLen[i], cudaMemcpyHostToDevice));
        }

        // device 端 value 指针数组
        const char** d_values = nullptr;
        CUDA_CHK(cudaMalloc(&d_values, N * sizeof(char*)));
        CUDA_CHK(cudaMemcpy(d_values, h_dValuePtrs.data(), N*sizeof(char*), cudaMemcpyHostToDevice));

        // 为输出分配 device 缓冲，每条 MAX_LEN
        vector<char*> h_dOutPtrs(N);
        for (int i = 0; i < N; ++i) {
            CUDA_CHK(cudaMalloc(&h_dOutPtrs[i], MAX_LEN));
        }
        char** d_out = nullptr;
        CUDA_CHK(cudaMalloc(&d_out, N * sizeof(char*)));
        CUDA_CHK(cudaMemcpy(d_out, h_dOutPtrs.data(), N*sizeof(char*), cudaMemcpyHostToDevice));

        // ---------------- kernel launch ----------------------
        int block = 256;
        int grid  = (N + block - 1) / block;
        generate_single_segment_kernel<<<grid, block>>>(d_values, d_out, N);
        CUDA_CHK(cudaDeviceSynchronize());

        // ---------------- 取回结果 ---------------------------
        char h_buf[MAX_LEN];
        for (int i = 0; i < N; ++i) {
            CUDA_CHK(cudaMemcpy(h_buf, h_dOutPtrs[i], MAX_LEN, cudaMemcpyDeviceToHost));
            guesses.emplace_back(h_buf);
            total_guesses++;
        }

        // ---------------- 释放显存 ---------------------------
        for (int i = 0; i < N; ++i) {
            cudaFree(h_dValuePtrs[i]);
            cudaFree(h_dOutPtrs[i]);
        }
        cudaFree(d_values);
        cudaFree(d_out);
#else
        // 无 CUDA 宏时，退回 CPU for 循环
        for (int i = 0; i < N; ++i) {
            guesses.emplace_back(a->ordered_values[i]);
            ++total_guesses;
        }
#endif
    }
    // ------------------------------------------------------------------
    // 3. 多段 PT：prefix + 并行追加最后 segment
    // ------------------------------------------------------------------
    else
    {
        // 3.1 CPU 侧拼 prefix
        string prefix;
        for (size_t segIdx = 0; segIdx < pt.content.size() - 1; ++segIdx) {
            int idx = pt.curr_indices[segIdx];
            if (pt.content[segIdx].type == 1) prefix += m.letters[m.FindLetter(pt.content[segIdx])].ordered_values[idx];
            if (pt.content[segIdx].type == 2) prefix += m.digits [m.FindDigit (pt.content[segIdx])].ordered_values[idx];
            if (pt.content[segIdx].type == 3) prefix += m.symbols[m.FindSymbol(pt.content[segIdx])].ordered_values[idx];
        }

        // 3.2 取得最后 segment 指针
        segment* a = nullptr;
        int lastType = pt.content.back().type;
        if (lastType == 1) a = &m.letters[m.FindLetter(pt.content.back())];
        if (lastType == 2) a = &m.digits [m.FindDigit (pt.content.back())];
        if (lastType == 3) a = &m.symbols[m.FindSymbol(pt.content.back())];

        const int N = pt.max_indices.back();
        if (N == 0) return;

#ifdef __CUDACC__
        // ---------------- device 前缀 ------------------------
        char* d_prefix = nullptr;
        CUDA_CHK(cudaMalloc(&d_prefix, prefix.size()+1));
        CUDA_CHK(cudaMemcpy(d_prefix, prefix.c_str(), prefix.size()+1, cudaMemcpyHostToDevice));

        // ---------------- device value 指针 ------------------
        vector<const char*> h_valPtrs(N);
        vector<size_t>      h_valLen (N);
        for (int i = 0; i < N; ++i) {
            h_valPtrs[i] = a->ordered_values[i].c_str();
            h_valLen [i] = a->ordered_values[i].size() + 1;
        }
        vector<char*> h_dValuePtrs(N);
        for (int i = 0; i < N; ++i) {
            CUDA_CHK(cudaMalloc(&h_dValuePtrs[i], h_valLen[i]));
            CUDA_CHK(cudaMemcpy(h_dValuePtrs[i], h_valPtrs[i], h_valLen[i], cudaMemcpyHostToDevice));
        }
        const char** d_values = nullptr;
        CUDA_CHK(cudaMalloc(&d_values, N*sizeof(char*)));
        CUDA_CHK(cudaMemcpy(d_values, h_dValuePtrs.data(), N*sizeof(char*), cudaMemcpyHostToDevice));

        // ---------------- device 输出指针 --------------------
        vector<char*> h_dOutPtrs(N);
        for (int i = 0; i < N; ++i) {
            CUDA_CHK(cudaMalloc(&h_dOutPtrs[i], MAX_LEN));
        }
        char** d_out = nullptr;
        CUDA_CHK(cudaMalloc(&d_out, N*sizeof(char*)));
        CUDA_CHK(cudaMemcpy(d_out, h_dOutPtrs.data(), N*sizeof(char*), cudaMemcpyHostToDevice));

        // ---------------- kernel -----------------------------
        int block = 256;
        int grid  = (N + block - 1) / block;
        generate_with_prefix_kernel<<<grid, block>>>(d_prefix, static_cast<int>(prefix.size()),
                                                     d_values, d_out, N);
        CUDA_CHK(cudaDeviceSynchronize());

        // ---------------- 拷回结果 ---------------------------
        char h_buf[MAX_LEN];
        for (int i = 0; i < N; ++i) {
            CUDA_CHK(cudaMemcpy(h_buf, h_dOutPtrs[i], MAX_LEN, cudaMemcpyDeviceToHost));
            guesses.emplace_back(h_buf);
            ++total_guesses;
        }

        // ---------------- 释放 -------------------------------
        for (int i = 0; i < N; ++i) {
            cudaFree(h_dValuePtrs[i]);
            cudaFree(h_dOutPtrs[i]);
        }
        cudaFree(d_values);
        cudaFree(d_out);
        cudaFree(d_prefix);
#else
        // 无 CUDA 时使用原 CPU 循环
        for (int i = 0; i < N; ++i) {
            guesses.emplace_back(prefix + a->ordered_values[i]);
            ++total_guesses;
        }
#endif
    }
}
