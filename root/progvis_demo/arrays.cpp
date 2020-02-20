struct my_type {
	int a;
	int b;
	int c;

	my_type() : a(1), b(2), c(3) {}
};

int main() {
	my_type x;

	for (int *a = &x.a; a <= &x.c; a += 1) {
		(*a)++;
	}

	return 0;
}
