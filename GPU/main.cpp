#include "PCFG.h"
#include <chrono>
#include <fstream>
#include "md5.h"
#include <iomanip>
#include <unordered_set>
using namespace std;
using namespace chrono;

// 编译指令如下
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main -O1
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main -O2
//nvcc main.cpp guessing2.cu md5.cpp -o test
//nvcc -o main main.cpp guessing2.cpp guessing.cu md5.cpp -O2
//nvcc -O3 -std=c++17 guessing.cu main.cpp train.cpp md5.cpp -o demo
//nvcc -O2 -std=c++17 guessing.cu guessing2.cpp train.cpp md5.cpp main.cpp -o demo
int main()
{
    double time_train = 0;
    double time_generate = 0;
    double time_hash = 0;

    PriorityQueue q;

    // ==== 1. 模型训练阶段 ====
    auto start_train = system_clock::now();
    q.m.train("/home/s2213230/sum/guessdata/Rockyou-singleLined-full.txt");
    q.m.order();
    auto end_train = system_clock::now();
    time_train = duration_cast<duration<double>>(end_train - start_train).count();

    // ==== 2. 加载测试集（用于 crack 检验）====
    unordered_set<string> test_set;
    ifstream test_data("/home/s2213230/sum/guessdata/Rockyou-singleLined-full.txt");
    string pw;
    int test_count = 0;
    while (test_data >> pw)
    {
        test_set.insert(pw);
        if (++test_count >= 1000000) break;
    }

    // ==== 3. 初始化优先队列 ====
    q.init();
    cout << "here" << endl;

    int cracked = 0;
    int curr_num = 0;
    int history = 0;

    // ==== 4. 主循环：生成、哈希、统计 ====
    while (!q.priority.empty())
    {
        // ===== 生成时间统计 =====
        int batch_size = 100;
        auto start_generate = system_clock::now();
        q.PopNext(batch_size);  // 包含 Generate()，内部调用 GPU
        auto end_generate = system_clock::now();
        time_generate += duration_cast<duration<double>>(end_generate - start_generate).count();

        q.total_guesses = q.guesses.size();

        // 输出生成进度
        if (q.total_guesses - curr_num >= 100000)
        {
            cout << "Guesses generated: " << history + q.total_guesses << endl;
            curr_num = q.total_guesses;

            if (history + q.total_guesses >= 10000000)
            {
                cout << fixed << setprecision(6);
                cout << "Guess time: " << time_generate << " seconds" << endl;
                cout << "Hash time: " << time_hash << " seconds" << endl;
                cout << "Train time: " << time_train << " seconds" << endl;
                cout << "Cracked: " << cracked << endl;
                break;
            }
        }

        // ===== 达到 1M 条后进行哈希处理 =====
        if (curr_num >= 1000000)
        {
            auto start_hash = system_clock::now();
            bit32 state[4];

            for (const string &pw : q.guesses)
            {
                if (test_set.find(pw) != test_set.end()) {
                    cracked++;
                }
                MD5Hash(pw, state);
                // 可选：打印结果
                // cout << pw << "\t";
                // for (int i = 0; i < 4; ++i)
                //     cout << setw(8) << setfill('0') << hex << state[i];
                // cout << endl;
            }

            auto end_hash = system_clock::now();
            time_hash += duration_cast<duration<double>>(end_hash - start_hash).count();

            history += curr_num;
            curr_num = 0;
            q.guesses.clear();
        }
    }

    return 0;
}