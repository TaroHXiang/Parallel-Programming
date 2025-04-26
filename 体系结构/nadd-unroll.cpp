#include <iostream>
#include <ctime>
using namespace std;

int main() {
    int n;
    cout << "Input n:";
    cin >> n;

    // 内存分配与初始化
    int* a = new int[n];
    for (int i = 0; i < n; i++) {
        a[i] = i;
    }

    // 性能测试变量
    float seconds;
    clock_t start, finish;
    long counter = 0;
    start = clock();

    // 展开优化变量
    int sum = 0;
    const int UNROLL_FACTOR = 8;  // 8路展开
    int partial_sums[UNROLL_FACTOR] = { 0 };

    while (clock() - start < 5) {  // 5秒测试窗口
        counter++;
        sum = 0;
        // 重置部分和数组
        for (int k = 0; k < UNROLL_FACTOR; k++) {
            partial_sums[k] = 0;
        }

        // 主循环（8路展开）
        int i = 0;
        for (; i <= n - UNROLL_FACTOR; i += UNROLL_FACTOR) {
            partial_sums[0] += a[i];
            partial_sums[1] += a[i + 1];
            partial_sums[2] += a[i + 2];
            partial_sums[3] += a[i + 3];
            partial_sums[4] += a[i + 4];
            partial_sums[5] += a[i + 5];
            partial_sums[6] += a[i + 6];
            partial_sums[7] += a[i + 7];
        }

        // 处理剩余元素（n % UNROLL_FACTOR）
        for (; i < n; i++) {
            partial_sums[0] += a[i];
        }

        // 合并部分和
        for (int k = 0; k < UNROLL_FACTOR; k++) {
            sum += partial_sums[k];
        }

        // 性能输出
        finish = clock();
        seconds = (finish - start) / float(CLOCKS_PER_SEC);
        cout << "Size:" << n
            << " Executions:" << counter
            << " Runtime:" << seconds
            << " Per-run:" << seconds / counter << endl;
    }

    delete[] a;
    return 0;
}
