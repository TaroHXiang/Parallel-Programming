#include<iostream>
#include<ctime>
using namespace std;

int main() {
    int n;
    cout << "Input n:";
    cin >> n;

    // 内存分配
    int** matri1 = new int* [n];
    int* column_sum = new int[n];
    int* vec = new int[n];

    // 初始化矩阵和向量
    for (int i = 0; i < n; i++) {
        matri1[i] = new int[n];
        for (int j = 0; j < n; j++) {
            matri1[i][j] = i + j;
        }
        vec[i] = i;
    }

    float seconds;
    clock_t start, finish;
    long counter = 0;
    start = clock();

    while (clock() - start < 10) {  // 运行10秒
        counter++;

        // 清零列和数组
        for (int i = 0; i < n; i++) {
            column_sum[i] = 0;
        }

        for (int j = 0; j < n; j++) {
            int vec_val = vec[j];  
            int* mat_col = &matri1[j][0];  

            int i = 0;
            for (i; i <= n - 4; i += 4) {
                column_sum[i] += mat_col[i] * vec_val;
                column_sum[i + 1] += mat_col[i + 1] * vec_val;
                column_sum[i + 2] += mat_col[i + 2] * vec_val;
                column_sum[i + 3] += mat_col[i + 3] * vec_val;
            }
            // 处理剩余元素
            for (; i < n; i++) {
                column_sum[i] += mat_col[i] * vec_val;
            }
        }

        finish = clock();
        seconds = (finish - start) / float(CLOCKS_PER_SEC);
        cout << "Matrix size:" << n << " "
            << "Number of executions:" << counter << " "
            << "Runtime:" << seconds << " "
            << "Per-runtime:" << seconds / counter << endl;
    }

    // 释放内存
    for (int i = 0; i < n; i++) {
        delete[] matri1[i];
    }
    delete[] matri1;
    delete[] column_sum;
    delete[] vec;

    return 0;
}