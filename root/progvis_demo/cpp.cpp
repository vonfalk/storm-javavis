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
	int *c = &*b; // Don't know if this is valid C++...
	*c = 9;
	return a;
}

int ptrs2() {
	demo d;
	int *x = &d.x;
	d.x = 8;
	return *x;
}

int main() {
	demo d;
	d.x = 8;
	d.y = 10;

	print(test(2 + 3, 4));
	print(d.result());
	print(ptrs());
	print(ptrs2());

	return 1 + d.x;
}
