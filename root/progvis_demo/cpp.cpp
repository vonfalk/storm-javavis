#include <iostream>

class demo {
public:
	int x;
	int y;

	int result() {
		return 3;
	}
};

int test(int a, int b) {
	int c = a++;
	int d = ++a;
	return a + b + c + d;
}

int ptrs() {
	int a = 8;
	int *b = &a;
	// *b = 9;
	return a;
}

int main() {
	demo d;
	d.x = 8;
	d.y = 10;

	print(test(2 + 3, 4));
	print(d.result());
	print(ptrs());

	return 1 + d.x;
}
