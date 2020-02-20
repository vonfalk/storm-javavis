struct my_type {
	int a;
	int b;
	int c;

	my_type() : a(1), b(2), c(3) {}
};

int main() {
	my_type x;

	for (int *a = &x.a; a <= &x.c; ++a) {
		(*a)++;
	}

	print(&x.c - &x.a);
	print(sizeof(my_type));
	print(sizeof(my_type *));

	return 0;
}
