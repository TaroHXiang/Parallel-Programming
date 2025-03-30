#include<iostream>
#include<time.h>
using namespace std;
int main2() {
	int n;
	cout << "Input n:";
	cin >> n;
	int** matri1 = new int* [n];
	int* column_sum = new int[n];
	int* vec = new int[n];
	for (int i = 0;i < n;i++) {
		matri1[i] = new int[n];
	}
	for (int i = 0;i < n;i++) {
		for (int j = 0;j < n;j++) {
			matri1[i][j] = i + j;
		}
	}
	for (int i = 0;i < n;i++) {
		vec[i] = i;
	}
	float seconds;
	clock_t start, finish;
	long counter = 0;
	start = clock();
	while (clock() - start < 10) {
		counter++;
		for (int i = 0; i < n; i++)
			column_sum[i] = 0;
		for (int j = 0; j < n; j++) {
			for (int i = 0; i < n; i++)
				column_sum[i] += matri1[j][i] * vec[j];
		}
		finish = clock();
		seconds = (finish - start) / float(CLOCKS_PER_SEC);
		cout << "Matrix size:" << n << " " << "Number of executions:" << counter << " " << "Runtime:" << seconds << " " << "Per-runtime:" << seconds / counter << endl;
	}
	return 0;
}