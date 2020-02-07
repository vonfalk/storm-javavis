void fn(int a) {
	print(8);
}

void fn(long a) {
	print(10);
}

void fn2(int &a) {
	// print(a);
}

int main() {
	int a = 10;
	long b = 10;
	fn(10);
	fn(a);
	fn(b);

	// fn2(a);

	return 0;
}
