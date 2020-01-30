#include <iostream>

class demo {
public:
	int x;
	int y;

	int result(int param) {
		return 3 + param;
	}

	int sum() {
		return this->x + y;
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

int ptrs3() {
	demo d;
	demo *p = &d;
	d.x = 10;
	return p->sum();
}

int main() {
	demo d;
	d.x = 8;
	d.y = 10;

	print(test(2 + 3, 4));
	print(d.result(1));
	print(d.sum());
	print(ptrs());
	print(ptrs2());
	print(ptrs3());

	return 1 + d.x;
}
