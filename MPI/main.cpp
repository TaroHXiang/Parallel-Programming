// main.cpp
#include "PCFG.h"
#include <chrono>
#include <fstream>
#include "md5.h"
#include <iomanip>
#include <unordered_set>
#include <mpi.h>
using namespace std;
using namespace chrono;
//mpic++ main.cpp train.cpp guessingMPI.cpp md5.cpp -o main -O2

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double time_hash = 0;
    double time_guess = 0;
    double time_train = 0;
    double guess_only_time = 0;
    PriorityQueue q;

    auto start_train = system_clock::now();
    q.m.train("/guessdata/Rockyou-singleLined-full.txt");
    q.m.order();
    auto end_train = system_clock::now();
    auto duration_train = duration_cast<microseconds>(end_train - start_train);
    time_train = double(duration_train.count()) * microseconds::period::num / microseconds::period::den;

    unordered_set<string> test_set;
    ifstream test_data("/guessdata/Rockyou-singleLined-full.txt");
    int test_count = 0;
    string pw;
    while (test_data >> pw)
    {
        test_count += 1;
        test_set.insert(pw);
        if (test_count >= 1000000) break;
    }

    int cracked = 0;
    q.init(rank, size);
    int curr_num = 0;
    auto start = system_clock::now();
    int history = 0;

    // 每个进程跳着处理 PT（避免重复）
    while (!q.priority.empty())
    {
        q.PopNext(rank, size);  // 假设你支持根据 rank 和 size 分布处理
        q.total_guesses = q.guesses.size();

        if (q.total_guesses - curr_num >= 100000)
        {
            curr_num = q.total_guesses;

            if (history + q.total_guesses > 10000000)
            {
                auto end = system_clock::now();
                auto duration = duration_cast<microseconds>(end - start);
                time_guess = double(duration.count()) * microseconds::period::num / microseconds::period::den;
                guess_only_time = time_guess - time_hash;
                break;
            }
        }

        if (curr_num > 1000000)
        {
            auto start_hash = system_clock::now();
            bit32 state[4];
            for (string pw : q.guesses)
            {
                if (test_set.find(pw) != test_set.end()) cracked++;
                MD5Hash(pw, state);
            }
            auto end_hash = system_clock::now();
            auto duration = duration_cast<microseconds>(end_hash - start_hash);
            time_hash += double(duration.count()) * microseconds::period::num / microseconds::period::den;

            history += curr_num;
            curr_num = 0;
            q.guesses.clear();
        }
    }

    // 统计阶段：每个进程把 cracked 数等发送给 rank 0
    int total_cracked = 0;
    double total_guess_only_time = 0;
    double total_train = 0, total_guess = 0, total_hash = 0;
    MPI_Reduce(&cracked, &total_cracked, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time_train, &total_train, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time_guess, &total_guess, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time_hash, &total_hash, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&guess_only_time, &total_guess_only_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);


    if (rank == 0)
    {
        cout << "Total Cracked: " << total_cracked << endl;
        cout << "Train time: " << total_train << " seconds" << endl;
        cout << "Guess-only time: " << total_guess_only_time << " seconds" << endl;
        cout << "Hash time: " << total_hash << " seconds" << endl;
    }

    MPI_Finalize();
    return 0;
}