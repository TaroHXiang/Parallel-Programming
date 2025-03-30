#include<iostream>
#include<time.h>
using namespace std;
int main4() {
	int n;
	cout << "Input n:";
	cin >> n;
	int* a = new int[n];
	int sum = 0;
	for (int i = 0;i < n;i++) {
		a[i] = i;
	}
	float seconds;
	clock_t start, finish;
	long counter = 0;
	start = clock();
	int sum1, sum2,sum3,sum4;
	while (clock() - start < 5) {
		counter++;
		sum1 = 0; sum2 = 0;sum3 = 0;sum4 = 0;
		for (int i = 0;i < n; i += 4) {
			 sum1 += a[i];
			 sum2 += a[i + 1];
			 sum3 += a[i + 2];
			 sum4 += a[i + 3];
	
		}
		sum = sum1 + sum2+sum3+sum4;
		finish = clock();
		seconds = (finish - start) / float(CLOCKS_PER_SEC);
		cout << "Matrix size:" << n << " " << "Number of executions:" << counter << " " << "Runtime:" << seconds << " " << "Per-runtime:" << seconds / counter << endl;
	}
	return 0;
}