#include <iostream>

int test(int a, int b) {
	return ++a + b++;
}

int main() {
	return 1 + test(2 + 3, 4);
}
