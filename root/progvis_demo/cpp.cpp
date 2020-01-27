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

int main() {
	demo d;
	d.x = 8;
	d.y = 10;

	return 1 + test(2 + 3, 4) + d.x;
}
