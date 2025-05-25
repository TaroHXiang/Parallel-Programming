#include "PCFG.h"
#include <chrono>
#include <fstream>
#include "md5.h"
#include <iomanip>
#include <unordered_set>
using namespace std;
using namespace chrono;

// 编译指令如下
// g++ -fopenmp main.cpp train.cpp guessingopenMP.cpp md5.cpp -o main2
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main -O1
// g++ main.cpp train.cpp guessing.cpp md5.cpp -o main -O2

int main()
{
    double time_hash = 0;
    double time_guess = 0;
    double time_train = 0;
    PriorityQueue q;

    auto start_train = system_clock::now();
    q.m.train("/guessdata/Rockyou-singleLined-full.txt");
    q.m.order();
    auto end_train = system_clock::now();
    time_train = duration_cast<duration<double>>(end_train - start_train).count();

    unordered_set<string> test_set;
    ifstream test_data("/guessdata/Rockyou-singleLined-full.txt");
    string pw;
    int test_count = 0;
    while (test_data >> pw && test_count < 1000000) {
        test_set.insert(pw);
        test_count++;
    }

    int cracked = 0;
    int history = 0;
    q.init();
    cout << "here" << endl;

    auto start = system_clock::now();
    const int BATCH_SIZE = 64;

    while (!q.priority.empty()) {
        // 1. 批量 Pop 多个 PT
        vector<PT> batch_pts;
        while (!q.priority.empty() && batch_pts.size() < BATCH_SIZE) {
            batch_pts.push_back(q.priority.front());
            q.priority.erase(q.priority.begin());
        }

        // 2. 并行 Generate 多个 PT
        #pragma omp parallel for
        for (int i = 0; i < batch_pts.size(); i++) {
            q.Generate(batch_pts[i]);
        }

        // 3. 并行哈希验证
        auto start_hash = system_clock::now();
        #pragma omp parallel for reduction(+:cracked)
        for (int i = 0; i < q.guesses.size(); i++) {
            string& pw = q.guesses[i];
            bit32 state[4];
            if (test_set.find(pw) != test_set.end()) {
                cracked++;
            }
            MD5Hash(pw, state);
        }
        auto end_hash = system_clock::now();
        time_hash += duration_cast<duration<double>>(end_hash - start_hash).count();

        history += q.guesses.size();
        q.guesses.clear();
        
        int generate_n=100000000;
        if (history >= 100000000) {
            auto end = system_clock::now();
            time_guess = duration_cast<duration<double>>(end - start).count();
            cout << "Guess time: " << (time_guess - time_hash) << " seconds" << endl;
            cout << "Hash time : " << time_hash << " seconds" << endl;
            cout << "Train time: " << time_train << " seconds" << endl;
            cout << "Cracked   : " << cracked << endl;
            break;
        }
    }
}
