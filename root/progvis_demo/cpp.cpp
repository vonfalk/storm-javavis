#include <iostream>

int test(int a, int b) {
	int c = a++;
	int d = ++a;
	return a + b + c + d;
}

int main() {
	return 1 + test(2 + 3, 4);
}
