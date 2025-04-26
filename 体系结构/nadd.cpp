#include<iostream>
#include<time.h>
using namespace std;
int main3() {
	int n;
	cout << "Input n:";
	cin >> n;
	int* a = new int [n];
	int sum=0;
	for (int i = 0;i < n;i++) {
		a[i] = i;
	}
	float seconds;
	clock_t start, finish;
	long counter = 0;
	start = clock();
	while (clock() - start < 5) {
		counter++;
		for (int i = 0;i < n;i++) {
			sum += a[i];
		}
		finish = clock();
		seconds = (finish - start) / float(CLOCKS_PER_SEC);
		cout << "Matrix size:" << n << " " << "Number of executions:" << counter << " " << "Runtime:" << seconds << " " << "Per-runtime:" << seconds / counter << endl;
	}
	return 0;
}
